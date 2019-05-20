#include "main_loop.h"
#include <chrono>
#include <memory>
#include <fmt/format.h>
#include "backend/Event.h"
#include "backend/Display.h"
#include "render/RenderAlgorithm.h"
#include "render/SvoRaytraceAlgorithm.h"
#include "render/DdaRaytraceAlgorithm.h"
#include "render/Renderer.h"
#include "core/Logger.h"
#include "core/Error.h"
#include "model/Grid.h"
#include "model/Octree.h"
#include "resources.h"

namespace {
    void check_setup(Display* display) {
        size_t num_devices = display->num_render_devices();
        if (num_devices == 0) {
            throw Error("Invalid setup (no render devices)");
        }

        auto buf = fmt::memory_buffer();

        fmt::format_to(buf, "Setup: {} {}, with ", num_devices, num_devices > 1 ? "devices" : "device");

        for (size_t i = 0; i < num_devices; ++i) {
            size_t outputs = display->render_device(i).outputs;
            if (outputs == 0) {
                throw Error("Invalid setup (device {} has no outputs)", i);
            }
            if (i == 0) {
                fmt::format_to(buf, "{}", outputs);
            } else {
                fmt::format_to(buf, ", {}", outputs);
            }
        }

        fmt::format_to(buf, " {}", num_devices > 1 || display->render_device(0).outputs > 1 ? "outputs" : "output");
        LOGGER.log(fmt::to_string(buf));
    }

    enum class FileType {
        Tiff,
        Svo,
        Unknown
    };

    FileType parse_file_type(std::string_view str) {
        if (str == "tiff" || str == "tif") {
            return FileType::Tiff;
        } else if (str == "svo") {
            return FileType::Svo;
        }

        return FileType::Unknown;
    }

    FileType guess_file_type(const RenderParameters& render_params) {
        if (!render_params.model_type_override.empty()) {
            return parse_file_type(render_params.model_type_override);
        }

        std::string_view extension = render_params.model_path.extension().native();
        if (extension.empty()) {
            return FileType::Unknown;
        }

        // Move past the dot
        extension.remove_prefix(1);

        return parse_file_type(extension);
    }

    std::unique_ptr<RenderAlgorithm> create_render_algorithm(const RenderParameters& render_params) {
        FileType ft = guess_file_type(render_params);

        switch (ft) {
            case FileType::Tiff: {
                auto grid = std::make_shared<Grid>(Grid::load_tiff(render_params.model_path));
                return std::make_unique<DdaRaytraceAlgorithm>(grid);
            }
            case FileType::Svo: {
                auto octree = std::make_shared<Octree>(Octree::load_svo(render_params.model_path));
                return std::make_unique<SvoRaytraceAlgorithm>(resources::open("resources/svo_laine.comp"), octree);
            }
            case FileType::Unknown:
                throw Error("Failed to parse model file type");
        }
    }
}

void main_loop(EventDispatcher& dispatcher, Display* display, const RenderParameters& render_params) {
    check_setup(display);

    auto shader_params = Renderer::ShaderParameters {
        .density = render_params.density
    };

    auto algo = create_render_algorithm(render_params);
    auto renderer = Renderer(display, algo.get(), shader_params);

    bool quit = false;
    dispatcher.bind_close([&quit] {
        quit = true;
    });

    dispatcher.bind(Key::Escape, [&quit](Action) {
        quit = true;
    });

    dispatcher.bind_swapchain_recreate([&renderer](size_t device, size_t output) {
        LOGGER.log("Resizing device {}, output {}", device, output);
        renderer.recreate(device, output);
    });

    auto start = std::chrono::high_resolution_clock::now();
    size_t frames = 0;

    LOGGER.log("Starting render loop...");
    while (!quit) {
        ++frames;

        renderer.render();

        auto now = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration<double>(now - start);

        if (diff > std::chrono::seconds{5}) {
            LOGGER.log("FPS: {}", static_cast<double>(frames) / diff.count());
            frames = 0;
            start = now;
        }

        display->poll_events();
    }
}

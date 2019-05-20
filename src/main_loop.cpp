#include "main_loop.h"
#include <chrono>
#include <memory>
#include <array>
#include <cassert>
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
    enum class FileType {
        Tiff,
        Svo,
        Unknown
    };

    struct ShaderOption {
        std::string_view option;
        FileType required_type;
        std::string_view source;
    };

    constexpr const auto shader_options = std::array {
        ShaderOption{"dda", FileType::Tiff, resources::open("resources/dda.comp")},
        ShaderOption{"svo-naive", FileType::Svo, resources::open("resources/svo.comp")},
        ShaderOption{"svo-laine", FileType::Svo, resources::open("resources/svo_laine.comp")},
        ShaderOption{"svo-2", FileType::Svo, resources::open("resources/svo2.comp")},
    };

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

    std::string_view file_type_to_string(FileType ft) {
        switch (ft) {
            case FileType::Tiff:
                return "tiff";
            case FileType::Svo:
                return "svo";
            case FileType::Unknown:
                return "unknown";
        }
    }

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

    const ShaderOption& select_shader(const RenderParameters& render_params, FileType model_type) {
        if (!render_params.shader.empty()) {
            for (const auto& opt : shader_options) {
                if (opt.option == render_params.shader) {
                    if (opt.required_type == model_type) {
                        return opt;
                    } else {
                        throw Error(
                            "Shader '{}' is incompatible with model type '{}' (requires '{}')",
                            opt.option,
                            file_type_to_string(model_type),
                            file_type_to_string(opt.required_type)
                        );
                    }
                }
            }

            throw Error("Invalid shader '{}'", render_params.shader);
        } else {
            // Select a default: the first one of the right file type appearing in the shader_options list

            for (const auto& opt : shader_options) {
                if (opt.required_type == model_type) {
                    return opt;
                }
            }

            assert(false); // unreachable
        }
    }

    std::unique_ptr<RenderAlgorithm> create_render_algorithm(const RenderParameters& render_params) {
        FileType model_type = guess_file_type(render_params);
        if (model_type == FileType::Unknown) {
            throw Error("Failed to parse model file type");
        }

        LOGGER.log("Model file type: '{}'", file_type_to_string(model_type));
        const ShaderOption shader = select_shader(render_params, model_type);
        LOGGER.log("Using shader '{}'", shader.option);

        switch (model_type) {
            case FileType::Tiff: {
                // There is only one DDA shader, so that should always be picked here
                auto grid = std::make_shared<Grid>(Grid::load_tiff(render_params.model_path));
                return std::make_unique<DdaRaytraceAlgorithm>(grid);
            }
            case FileType::Svo: {
                auto octree = std::make_shared<Octree>(Octree::load_svo(render_params.model_path));
                return std::make_unique<SvoRaytraceAlgorithm>(shader.source, octree);
            }
            default:
                assert(false); // make compiler happy
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

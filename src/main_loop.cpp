#include "main_loop.h"
#include <chrono>
#include <memory>
#include <array>
#include <algorithm>
#include <cassert>
#include <fmt/format.h>
#include "backend/Event.h"
#include "backend/Display.h"
#include "render/RenderAlgorithm.h"
#include "render/SvoRaytraceAlgorithm.h"
#include "render/DdaRaytraceAlgorithm.h"
#include "render/Renderer.h"
#include "camera/Camera.h"
#include "camera/OrbitCameraController.h"
#include "camera/BenchmarkCameraController.h"
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

    constexpr const auto SHADER_OPTIONS = std::array {
        ShaderOption{"dda", FileType::Tiff, resources::open("resources/dda.comp")},
        ShaderOption{"svo-naive", FileType::Svo, resources::open("resources/svo_naive.comp")},
        ShaderOption{"esvo", FileType::Svo, resources::open("resources/esvo.comp")},
        ShaderOption{"svo-2", FileType::Svo, resources::open("resources/svo2.comp")},
        ShaderOption{"svo-rope", FileType::Svo, resources::open("resources/svo_rope.comp")}
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
            default:
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
            auto it = std::find_if(SHADER_OPTIONS.begin(), SHADER_OPTIONS.end(), [&](const auto& opt) {
                return opt.option == render_params.shader;
            });

            if (it == SHADER_OPTIONS.end()) {
                throw Error("Invalid shader '{}'", render_params.shader);
            } else if (it->required_type == model_type) {
                return *it;
            } else {
                throw Error(
                    "Shader '{}' is incompatible with model type '{}' (requires '{}')",
                    it->option,
                    file_type_to_string(model_type),
                    file_type_to_string(it->required_type)
                );
            }
        } else {
            // Select a default: the first one of the right file type appearing in the SHADER_OPTIONS list

            for (const auto& opt : SHADER_OPTIONS) {
                if (opt.required_type == model_type) {
                    return opt;
                }
            }

            assert(false); // unreachable
        }
    }

    struct CreateRenderAlgorithmResult {
        std::unique_ptr<RenderAlgorithm> algo;
        Vec3Sz model_dim;
    };

    CreateRenderAlgorithmResult create_render_algorithm(const RenderParameters& render_params) {
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
                return {
                    std::make_unique<DdaRaytraceAlgorithm>(grid),
                    grid->dimensions()
                };
            }
            case FileType::Svo: {
                auto octree = std::make_shared<Octree>(Octree::load_svo(render_params.model_path));
                return {
                    std::make_unique<SvoRaytraceAlgorithm>(shader.source, octree),
                    Vec3Sz(octree->side())
                };
            }
            default:
                assert(false); // make compiler happy
        }
    }

    std::unique_ptr<CameraController> create_camera_controller(EventDispatcher& dispatcher, const RenderParameters& render_params) {
        switch (render_params.camera_type) {
            case CameraType::Orbit: {
                // LOGGER.log("Using orbit camera controller: use w/s to change pitch, a/d to change yaw, q/e to change roll and up/down to zoom");
                LOGGER.log("Using orbit camera controller. Controlls: ");
                LOGGER.log(" - w/s to change pitch");
                LOGGER.log(" - a/d to change yaw");
                LOGGER.log(" - q/e to change roll");
                LOGGER.log(" - up/down to zoom");
                return std::make_unique<OrbitCameraController>(dispatcher);
            }
            case CameraType::Benchmark: {
                LOGGER.log("Using benchmark camera controller");
                return std::make_unique<BenchmarkCameraController>();
            }
        }
    }
}

void main_loop(EventDispatcher& dispatcher, Display* display, const RenderParameters& render_params) {
    check_setup(display);

    auto [algo, dim] = create_render_algorithm(render_params);
    LOGGER.log("Model dimensions: {}x{}x{}", dim.x, dim.y, dim.z);

    auto shader_params = Renderer::ShaderParameters {
        .voxel_ratio = Vec4F(render_params.voxel_ratio, 0),
        .model_dim = Vec4<unsigned>(static_cast<Vec3<unsigned>>(dim), 0),
        .density = render_params.density
    };

    auto renderer = Renderer(display, algo.get(), shader_params);

    auto controller = create_camera_controller(dispatcher, render_params);

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
    auto last_frame = start;
    size_t frames = 0;

    auto accum = RenderStatsAccumulator();
    accum.start();

    LOGGER.log("Starting render loop...");
    while (!quit) {
        ++frames;

        renderer.render(controller->camera());
        accum(renderer.stats());

        auto frame_end = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(frame_end - last_frame).count();
        if (controller->update(dt)) {
            break;
        }

        last_frame = frame_end;

        auto now = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration<double>(now - start);

        if (diff > std::chrono::seconds{5}) {
            LOGGER.log("FPS: {}", static_cast<double>(frames) / diff.count());
            frames = 0;
            start = now;
        }

        display->poll_events();
    }

    accum.stop();
    LOGGER.log(
        "total rays: {}, total render time: {}ms, mray/s: {}",
        accum.total_rays(),
        accum.total_render_time(),
        accum.mrays_per_s()
    );

    LOGGER.log(
        "frames: {}, fps: {}, total time: {}s",
        accum.frames(),
        accum.fps(),
        accum.total_time().count()
    );

    if (!render_params.stats_save_path.empty()) {
        accum.save(render_params.stats_save_path);
        LOGGER.log("Saved stats to '{}'", render_params.stats_save_path.native());
    }
}

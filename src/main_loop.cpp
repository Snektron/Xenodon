#include "main_loop.h"
#include <chrono>
#include <fmt/format.h>
#include "backend/Event.h"
#include "backend/Display.h"
#include "render/SvoRaytracer.h"
#include "render/DdaRaytracer.h"
#include "render/ComputeSvoRaytracer.h"
#include "core/Logger.h"
#include "core/Error.h"
#include "model/Grid.h"
#include "model/Octree.h"

#include <vector>
#include <lodepng.h>
#include "model/Pixel.h"

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
}

void main_loop(EventDispatcher& dispatcher, Display* display) {
    check_setup(display);

    // const auto grid = Grid::load_tiff("/home/robin/Downloads/ZF-Eye.tif");

    auto grid = Grid({256, 256, 256});

    auto set = [&](size_t x, size_t y, size_t z) {
        grid.set({x, y, z}, Pixel{static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(z), 255});
    };

    for (size_t a = 0; a < 256; ++a) {
        for (size_t b = 0; b < 256; ++b) {
            set(a, b, 255);
            set(a, 255, b);
            set(255, a, b);
        }
    }

    const auto octree = Octree(grid, 30, true);
    auto renderer = ComputeSvoRaytracer(display, octree);

    // auto renderer = DdaRaytracer(display, grid);

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

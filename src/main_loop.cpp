#include "main_loop.h"
#include <chrono>
#include <fmt/format.h>
#include "present/Event.h"
#include "present/Display.h"
#include "render/Renderer.h"
#include "core/Logger.h"

namespace {
    void report_setup(const Setup& setup) {
        size_t ngpus = setup.size();

        fmt::print("Setup: {}{}, with {}", ngpus, ngpus > 1 ? " gpus" : " gpu", setup[0]);

        for (size_t i = 1; i < setup.size(); ++i) {
            fmt::print(", {}", setup[i]);
        }

        fmt::print(" {}\n", ngpus > 1 || setup[0] > 1 ? " screens" : " screen");
    }
}

void main_loop(Logger& logger, EventDispatcher& dispatcher, Display* display) {
    auto setup = display->setup();
    if (setup.empty()) {
        fmt::print("Error: Invalid setup (no gpus)\n");
    }

    for (size_t num_screens : setup) {
        if (num_screens == 0) {
            fmt::print("Error: Invalid setup (no screens)\n");
        }
    }

    report_setup(setup);

    auto renderer = Renderer(display);

    bool quit = false;
    dispatcher.bind_close([&quit] {
        quit = true;
    });

    dispatcher.bind(Key::Escape, [&quit](Action) {
        quit = true;
    });

    dispatcher.bind_swapchain_recreate([&renderer](size_t gpu, size_t screen) {
        fmt::print("Resizing gpu {}, screen {}\n", gpu, screen);
        renderer.recreate(gpu, screen);
    });

    auto start = std::chrono::high_resolution_clock::now();
    size_t frames = 0;

    while (!quit) {
        ++frames;

        renderer.render();

        auto now = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration<double>(now - start);

        if (diff > std::chrono::seconds{1}) {
            fmt::print("FPS: {}\n", static_cast<double>(frames) / diff.count());
            frames = 0;
            start = now;
        }

        display->swap_buffers();
        display->poll_events();
    }
}

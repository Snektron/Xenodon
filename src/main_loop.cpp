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

        auto buf = fmt::memory_buffer();

        fmt::format_to(buf, "Setup: {}{}, with {}", ngpus, ngpus > 1 ? " gpus" : " gpu", setup[0]);

        for (size_t i = 1; i < setup.size(); ++i) {
            fmt::format_to(buf, ", {}", setup[i]);
        }

        fmt::format_to(buf, " {}", ngpus > 1 || setup[0] > 1 ? "screens" : "screen");
        LOGGER.log(fmt::to_string(buf));
    }
}

void main_loop(EventDispatcher& dispatcher, Display* display) {
    auto setup = display->setup();
    if (setup.empty()) {
        LOGGER.log("Error: Invalid setup (no gpus)");
        return;
    }

    for (size_t i = 0; i < setup.size(); ++i) {
        if (setup[i] == 0) {
            LOGGER.log("Error: Invalid setup (gpu {} has no screens)", i);
            return;
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
        LOGGER.log("Resizing gpu {}, screen {}", gpu, screen);
        renderer.recreate(gpu, screen);
    });

    auto start = std::chrono::high_resolution_clock::now();
    size_t frames = 0;

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

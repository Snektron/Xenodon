#include "main_loop.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include "present/Event.h"
#include "present/Display.h"

namespace {
    void report_setup(const Setup& setup) {
        size_t ngpus = setup.size();

        std::cout << "setup: " << ngpus << (ngpus > 1 ? " gpus" : " gpu") << ", with " << setup[0];

        for (size_t i = 1; i < setup.size(); ++i) {
            std::cout << ", " << setup[i];
        }

        std::cout << (ngpus > 1 || setup[0] > 1 ? " screens" : " screen") << std::endl;
    }
}

void main_loop(EventDispatcher& dispatcher, Display* display) {
    auto setup = display->setup();
    if (setup.empty()) {
        std::cout << "Error: Invalid setup (no gpus)" << std::endl;
    }

    for (size_t num_screens : setup) {
        if (num_screens == 0) {
            std::cout << "Error: Invalid setup (no screens)" << std::endl;
        }
    }

    report_setup(setup);

    bool quit = false;
    dispatcher.bind_close([&quit] {
        quit = true;
    });

    dispatcher.bind(Key::Escape, [&quit](Action) {
        quit = true;
    });

    auto start = std::chrono::high_resolution_clock::now();
    size_t frames = 0;

    while (!quit) {
        ++frames;

        auto now = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration<double>(now - start);

        if (diff > std::chrono::seconds{1}) {
            std::cout << "FPS: " << std::fixed << static_cast<double>(frames) / diff.count() << std::endl;
            frames = 0;
            start = now;
        }

        display->swap_buffers();
        display->poll_events();
    }
}

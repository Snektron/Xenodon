#include "main_loop.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include "present/Event.h"
#include "present/Display.h"

void main_loop(EventDispatcher& dispatcher, Display* display) {
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

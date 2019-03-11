#include <iostream>
#include <string_view>
#include <chrono>
#include <iomanip>
#include <array>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "graphics/DisplayConfig.h"
#include "present/Display.h"
#include "present/Event.h"
#include "present/xorg/XorgDisplay.h"
#include "present/Frame.h"
#include "resources.h"

namespace {
    constexpr const char* const APP_NAME = "Xenodon";
    constexpr const uint32_t APP_VERSION = VK_MAKE_VERSION(0, 0, 0);

    const auto APP_INFO = vk::ApplicationInfo(
        APP_NAME,
        APP_VERSION,
        nullptr,
        0,
        VK_API_VERSION_1_1
    );

    void print_help(const char* program_name) {
        std::cout
            << "Usage:\n    " << program_name << " [subcommand] [flags]\n\n"
            << resources::open("resources/help.txt") << std::endl;
    }

    void detect() {
        constexpr const std::array required_instance_extensions = {
            VK_KHR_DISPLAY_EXTENSION_NAME
        };

        auto instance = vk::createInstanceUnique(
            vk::InstanceCreateInfo(
                {},
                &APP_INFO,
                0,
                nullptr,
                required_instance_extensions.size(),
                required_instance_extensions.data()
            )
        );

        auto dc = DisplayConfig::auto_detect(instance.get());

        if (dc.gpus.empty()) {
            std::cout << "No displays detected" << std::endl;
        } else {
            std::cout << dc << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        std::cout << "Error: Subcommand required, see `" << argv[0] << " help`" << std::endl;
        return 0;
    }

    auto subcommand = std::string_view(argv[1]);

    if (subcommand == "help") {
        print_help(argv[0]);
        return 0;
    } else if (subcommand == "detect") {
        detect();
        return 0;
    } else if (subcommand == "test") {
    } else {
        std::cout << "Error: Invalid subcommand '" << subcommand << "', see `" << argv[0] << " help`" << std::endl;
        return 0;
    }

    auto instance = vk::createInstanceUnique(
        vk::InstanceCreateInfo(
            {},
            &APP_INFO,
            0,
            nullptr,
            XorgDisplay::REQUIRED_INSTANCE_EXTENSIONS.size(),
            XorgDisplay::REQUIRED_INSTANCE_EXTENSIONS.data()
        )
    );

    auto dispatcher = EventDispatcher();
    auto display = static_cast<std::unique_ptr<Display>>(
        std::make_unique<XorgDisplay>(instance.get(), dispatcher, 800, 600)
    );

    bool quit = false;
    dispatcher.bind_close([&quit] {
        quit = true;
    });

    dispatcher.bind(Key::Escape, [&quit](Action) {
        quit = true;
    });

    dispatcher.bind(Key::A, [](Action a) {
        std::cout << "oof " << (a == Action::Press ? "Press" : "Release") << std::endl;
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

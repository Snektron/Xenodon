#include <iostream>
#include <cstddef>
#include <string_view>
#include <vulkan/vulkan.hpp>
#include "present/Display.h"
#include "present/Event.h"
#include "present/xorg/XorgDisplay.h"
#include "resources.h"

namespace {
    constexpr const char* const APP_NAME = "Xenodon";
    constexpr const uint32_t APP_VERSION = VK_MAKE_VERSION(0, 0, 0);

    constexpr const std::array INSTANCE_EXTENSIONS = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
        VK_KHR_DISPLAY_EXTENSION_NAME
    };

    void print_help(const char* program_name) {
        std::cout << "Usage: " << program_name << " [options]\n\n"
            << resources::open("resources/options.txt") << std::endl;
    }
}

int main(int argc, char* argv[]) {
    for (size_t i = 1; i < static_cast<size_t>(argc); ++i) {
        auto arg = std::string_view(argv[i]);
        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else {
            std::cerr << "Invalid argument '" << arg << "'.\nUse "
                << argv[0] << " --help for more information on usage." << std::endl;
            return 0;
        }
    }

    const auto app_info = vk::ApplicationInfo(
        APP_NAME,
        APP_VERSION,
        nullptr,
        0,
        VK_API_VERSION_1_1
    );

    auto instance = vk::createInstanceUnique(
        vk::InstanceCreateInfo(
            {},
            &app_info,
            0,
            nullptr,
            INSTANCE_EXTENSIONS.size(),
            INSTANCE_EXTENSIONS.data()
        )
    );

    auto dispatcher = EventDispatcher();
    auto display = std::unique_ptr<Display>(new XorgDisplay(instance.get(), dispatcher, 800, 600));

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

    while (!quit) {
        display->poll_events();
    }
}
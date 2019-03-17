#include "present/direct/direct_main.h"
#include <iostream>
#include <fstream>
#include "present/direct/DisplayConfig.h"
#include "present/direct/DirectDisplay.h"
#include "present/Event.h"
#include "main_loop.h"
#include "version.h"

namespace {
    constexpr const std::array REQUIRED_INSTANCE_EXTENSIONS = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_DISPLAY_EXTENSION_NAME,
    };
}

void direct_main(int argc, char* argv[]) {
    if (argc == 0) {
        std::cout << "Error: Missing argument <display config>" << std::endl;
        return;
    }

    auto in = std::ifstream(argv[0]);
    if (!in) {
        std::cout << "Error: Failed to open display config file '" << argv[0] << '\'' << std::endl;
        return;
    }

    auto instance = vk::createInstanceUnique(
        vk::InstanceCreateInfo(
            {},
            &version::APP_INFO,
            0,
            nullptr,
            REQUIRED_INSTANCE_EXTENSIONS.size(),
            REQUIRED_INSTANCE_EXTENSIONS.data()
        )
    );

    try {
        auto config = DisplayConfig(in);
        auto dispatcher = EventDispatcher();
        auto display = DirectDisplay(instance.get(), config);

        main_loop(dispatcher, &display);
    } catch (const DisplayConfig::ParseError& err) {
        std::cout << "Error: Failed to parse config file '" << argv[0] << "':\n"
            << err.what() << std::endl;
    }
}

#include <iostream>
#include <string_view>
#include <array>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "graphics/DisplayConfig.h"
#include "present/Display.h"
#include "present/Event.h"
#include "resources.h"
#include "main_loop.h"
#include "version.h"

#if defined(XENODON_PRESENT_XORG)
    #include "present/xorg/xorg_main.h"
#endif

namespace {
    void print_help(const char* program_name) {
        std::cout
            << "Usage:\n    " << program_name << " [subcommand] [flags]\n\n"
            << resources::open("resources/help.txt") << std::endl;
    }

    void detect() {
        constexpr const std::array required_instance_extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_DISPLAY_EXTENSION_NAME
        };

        auto instance = vk::createInstanceUnique(
            vk::InstanceCreateInfo(
                {},
                &version::APP_INFO,
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
    } else if (subcommand == "detect") {
        detect();
    } else if (subcommand == "xorg") {
        #if defined(XENODON_PRESENT_XORG)
            xorg_main(argc - 2, &argv[2]);
        #else
            std::cout << "Error: Xorg support was disabled" << std::endl;
        #endif
    } else if (subcommand == "direct") {
        #if defined(XENODON_PRESENT_DIRECT)
            std::cout << "Direct support is WIP" << std::endl;
        #else
            std::cout << "Error: Direct support was disabled" << std::endl;
        #endif
    } else {
        std::cout << "Error: Invalid subcommand '" << subcommand << "', see `" << argv[0] << " help`" << std::endl;
    }

    return 0;
}

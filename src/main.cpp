#include <string_view>
#include <array>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include <fmt/format.h>
#include "present/Display.h"
#include "present/Event.h"
#include "Config.h"
#include "resources.h"
#include "version.h"

#if defined(XENODON_PRESENT_XORG)
    #include "present/xorg/xorg_main.h"
#endif

#if defined(XENODON_PRESENT_DIRECT)
    #include "present/direct/direct_main.h"
#endif

namespace {
    void print_help(const char* program_name) {
        fmt::print(resources::open("resources/help.txt"), program_name);
    }

    void system_info(int argc, char* argv[]) {
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

        fmt::print("System setup information\n");
        auto gpus = instance->enumeratePhysicalDevices();
        if (gpus.empty()) {
            fmt::print("No GPUs detected\n");
            return;
        }

        for (size_t i = 0; i < gpus.size(); ++i) {
            auto props = gpus[i].getProperties();
            fmt::print(
                "GPU {}:\n\tname: '{}'\n\ttype: {}\n",
                i,
                props.deviceName,
                vk::to_string(props.deviceType)
            );

            auto display_props = gpus[i].getDisplayPropertiesKHR();

            if (display_props.empty()) {
                fmt::print("\tNo displays detected\n");
                continue;
            }

            for (size_t j = 0; j < display_props.size(); ++j) {
                fmt::print("\tDisplay {}:\n\t\tname: ", j);

                if (display_props[j].displayName) {
                    fmt::print("'{}'", display_props[j].displayName);
                } else {
                    fmt::print("(null)");
                }

                auto res = display_props[j].physicalResolution;
                fmt::print("\n\t\tresolution: {}x{}\n", res.width, res.height);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        fmt::print("Error: Subccommand required, see `{} help`\n", argv[0]);
        return 0;
    }

    auto subcommand = std::string_view(argv[1]);

    if (subcommand == "help") {
        print_help(argv[0]);
    } else if (subcommand == "sysinfo") {
        system_info(argc - 2, &argv[2]);
    } else if (subcommand == "xorg") {
        #if defined(XENODON_PRESENT_XORG)
            xorg_main(argc - 2, &argv[2]);
        #else
            fmt::print("Error: Xorg support was disabled\n");
        #endif
    } else if (subcommand == "direct") {
        #if defined(XENODON_PRESENT_DIRECT)
            direct_main(argc - 2, &argv[2]);
        #else
            fmt::print("Error: Direct support was disabled\n");
        #endif
    } else {
        fmt::print("Error: Invalid subcommand '{}', see `{} help`\n", subcommand, argv[0]);
    }

    return 0;
}

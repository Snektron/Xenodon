#include <iostream>
#include <string_view>
#include <array>
#include <cstddef>
#include <vulkan/vulkan.hpp>
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
        std::cout
            << "Usage:\n    " << program_name << " [subcommand]\n\n"
            << resources::open("resources/help.txt") << std::endl;
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

        auto str = [](const char* s) {
            if (s) {
                std::cout << '\'' << s << '\'';
            } else {
                std::cout << "(null)";
            }
        };

        std::cout << "System setup information:"; 
        auto gpus = instance->enumeratePhysicalDevices();
        if (gpus.empty()) {
            std::cout << "\nno gpus detected" << std::endl;
            return;
        }

        for (size_t i = 0; i < gpus.size(); ++i) {
            auto props = gpus[i].getProperties();
            std::cout << "\ngpu " << i << ":\n\tname: ";
            str(props.deviceName);
            std::cout << "\n\ttype: " << vk::to_string(props.deviceType);

            auto display_props = gpus[i].getDisplayPropertiesKHR();

            if (display_props.empty()) {
                std::cout << "\n\tno displays detected";
                continue;
            }

            for (size_t j = 0; j < display_props.size(); ++j) {
                std::cout << "\n\tdisplay " << j << ":\n\t\tname: ";
                str(display_props[j].displayName);
                auto res = display_props[j].physicalResolution;
                std::cout << "\n\t\tresolution: " << res.width << 'x' << res.height;
            }
        }

        std::cout << std::endl;
    }
}

struct Test {
    size_t x, y;
};

template <>
struct cfg::FromConfig<Test> {
    Test operator()(cfg::Config& cfg) {
        auto [x, y] = cfg.get(
            Value<size_t>("x"),
            Value<size_t>("y")
        );

        return {x, y};
    }
};

int main(int argc, char* argv[]) {
    std::stringstream ss;
    ss << R"(
oof = "ik heb aids"
test {
    x = 20
    y = 40
}

test {
    x = 30
    y = 50
}
)";

    cfg::Config cfg(ss);
    auto [oof, test] = cfg.root(cfg::Value<std::string>("oof"), cfg::Vector<Test>("test"));
    std::cout << "oof: " << oof << std::endl;

    for (auto& t : test) {
        std::cout << "test: {" << t.x << ", " << t.y << "}" << std::endl;
    }

    if (argc <= 1) {
        std::cout << "Error: Subcommand required, see `" << argv[0] << " help`" << std::endl;
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
            std::cout << "Error: Xorg support was disabled" << std::endl;
        #endif
    } else if (subcommand == "direct") {
        #if defined(XENODON_PRESENT_DIRECT)
            direct_main(argc - 2, &argv[2]);
        #else
            std::cout << "Error: Direct support was disabled" << std::endl;
        #endif
    } else {
        std::cout << "Error: Invalid subcommand '" << subcommand << "', see `" << argv[0] << " help`" << std::endl;
    }

    return 0;
}

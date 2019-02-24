#include <iostream>
#include <cstddef>
#include <string_view>
#include <vulkan/vulkan.hpp>
#include "interactive/interactive.h"
#include "resources.h"

namespace {
    constexpr const char* const APP_NAME = "Xenodon";
    constexpr const uint32_t APP_VERSION = VK_MAKE_VERSION(0, 0, 0);

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

    interactive_main(app_info);
}
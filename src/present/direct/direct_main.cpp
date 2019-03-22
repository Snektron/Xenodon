#include "present/direct/direct_main.h"
#include <fstream>
#include <fmt/format.h>
#include "core/Config.h"
#include "present/direct/DirectConfig.h"
#include "present/direct/DirectDisplay.h"
#include "present/Event.h"
#include "main_loop.h"
#include "version.h"

namespace {
    constexpr const std::array REQUIRED_INSTANCE_EXTENSIONS = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_DISPLAY_EXTENSION_NAME,
        VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME
    };
}

void direct_main(int argc, char* argv[]) {
    if (argc == 0) {
        fmt::print("Error: Missing argument <config>\n");
        return;
    }

    auto in = std::ifstream(argv[0]);
    if (!in) {
        fmt::print("Error: Failed to open config file '{}'\n", argv[0]);
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
        auto config = cfg::Config(in);
        auto direct_config = config.as<DirectConfig>();

        auto dispatcher = EventDispatcher();
        auto display = DirectDisplay(instance.get(), dispatcher, direct_config);

        main_loop(dispatcher, &display);
    } catch (const std::runtime_error& err) {
        fmt::print("Failed to read config file '{}':\n{}\n", argv[0], err.what());
    }
}

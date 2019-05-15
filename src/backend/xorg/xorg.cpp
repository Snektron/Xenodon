#include "backend/xorg/xorg.h"
#include <fstream>
#include "core/Error.h"
#include "core/Logger.h"
#include "core/Config.h"
#include "backend/Event.h"
#include "backend/xorg/XorgMultiGpuConfig.h"

std::unique_ptr<XorgDisplay> create_xorg_display(EventDispatcher& dispatcher, std::filesystem::path multi_gpu_config) {
    LOGGER.log("Using xorg presenting backend");

    if (multi_gpu_config.empty()) {
        return std::make_unique<XorgDisplay>(dispatcher, vk::Extent2D{800, 600});
    }


    auto in = std::ifstream(multi_gpu_config);
    if (!in) {
        throw Error("Failed to open config file '{}'", multi_gpu_config.native());
    }

    XorgMultiGpuConfig parsed_config;

    try {
        parsed_config = cfg::Config(in).as<XorgMultiGpuConfig>();
    } catch (const Error& err) {
        throw Error("Failed to read config file '{}': {}", multi_gpu_config.native(), err.what());
    }

    return std::make_unique<XorgDisplay>(dispatcher, parsed_config);
}

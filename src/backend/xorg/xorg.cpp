#include "backend/xorg/xorg.h"
#include <string_view>
#include <optional>
#include <fstream>
#include <vulkan/vulkan.hpp>
#include <fmt/format.h>
#include "core/Error.h"
#include "core/Logger.h"
#include "core/Config.h"
#include "backend/Event.h"
#include "backend/xorg/XorgMultiGpuConfig.h"

namespace {
    std::optional<XorgMultiGpuConfig> read_config(const char* file) {
        auto in = std::ifstream(file);
        if (!in) {
            fmt::print("Error: Failed to open config file '{}'\n", file);
            return std::nullopt;
        }

        try {
            return cfg::Config(in).as<XorgMultiGpuConfig>();
        } catch (const Error& err) {
            fmt::print("Failed to read config file '{}':\n{}\n", file, err.what());
            return std::nullopt;
        }
    }
}

std::unique_ptr<XorgDisplay> make_xorg_display(Span<const char*> args, EventDispatcher& dispatcher) {
    auto config = std::optional<XorgMultiGpuConfig>(std::nullopt);

    if (!args.empty()) {
        auto arg = std::string_view(args[0]);

        if (arg == "-m" || arg == "--multi-gpu") {
            if (args.size() == 1) {
                fmt::print("Error: {} requires argument <config>", arg);
            } else {
                config = read_config(args[1]);
                if (!config)
                    return nullptr;
            }
        } else {
            fmt::print("Error: Unrecognized argument '{}'\n", arg);
            return nullptr;
        }
    }

    LOGGER.log("Using xorg presenting backend");
    if (config) {
        return std::make_unique<XorgDisplay>(dispatcher, config.value());
    } else {
        return std::make_unique<XorgDisplay>(dispatcher, vk::Extent2D{800, 600});
    }
}

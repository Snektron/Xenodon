#include "backend/xorg/xorg.h"
#include <string_view>
#include <optional>
#include <fstream>
#include <vulkan/vulkan.hpp>
#include <fmt/format.h>
#include "core/Error.h"
#include "core/Logger.h"
#include "core/Config.h"
#include "core/arg_parse.h"
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
    const char* config_path = nullptr;

    auto cmd = args::Command {
        .parameters = {
            {config_path, "source type", "--multi-gpu", 'm'},
        }
    };

    if (!args::parse(args, cmd)) {
        return nullptr;
    }

    LOGGER.log("Using xorg presenting backend");
    if (config_path) {
        auto config = read_config(config_path);
        if (!config) {
            return nullptr;
        }
        return std::make_unique<XorgDisplay>(dispatcher, config.value());
    } else {
        return std::make_unique<XorgDisplay>(dispatcher, vk::Extent2D{800, 600});
    }
}

#include "present/direct/direct.h"
#include <fstream>
#include <optional>
#include <stdexcept>
#include <fmt/format.h>
#include "core/Config.h"
#include "core/Logger.h"
#include "present/direct/DirectConfig.h"
#include "present/Event.h"

namespace {
    std::optional<DirectConfig> read_config(const char* file) {
        auto in = std::ifstream(file);
        if (!in) {
            fmt::print("Error: Failed to open config file '{}'\n", std::string_view(file));
            return std::nullopt;
        }

        try {
            auto config = cfg::Config(in);
            return config.as<DirectConfig>();
        } catch (const std::runtime_error& err) {
            fmt::print("Failed to read config file '{}':\n{}\n", file, err.what());
            return std::nullopt;
        }
    }
}

std::unique_ptr<DirectDisplay> make_direct_display(Span<const char*> args, EventDispatcher& dispatcher) {
    if (args.empty()) {
        fmt::print("Error: Missing argument <config>\n");
        return nullptr;
    }

    auto config = read_config(args[0]);

    if (!config) {
        return nullptr;
    }

    LOGGER.log("Using direct presenting backend");
    return std::make_unique<DirectDisplay>(dispatcher, config.value());
}

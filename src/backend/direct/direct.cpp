#include "backend/direct/direct.h"
#include <fstream>
#include <stdexcept>
#include <fmt/format.h>
#include "core/Config.h"
#include "core/Logger.h"
#include "core/Error.h"
#include "core/arg_parse.h"
#include "backend/direct/DirectConfig.h"
#include "backend/Event.h"

std::unique_ptr<DirectDisplay> make_direct_display(Span<const char*> args, EventDispatcher& dispatcher) {
    const char* config_path = nullptr;
    auto cmd = args::Command {
        .positional = {
            {args::string_opt(&config_path), "config path"}
        }
    };

    try {
        args::parse(args, cmd);
    } catch (const args::ParseError& e) {
        fmt::print("Error: {}\n", e.what());
        return nullptr;
    }

    auto in = std::ifstream(config_path);
    if (!in) {
        fmt::print("Error: Failed to open config file '{}'\n", std::string_view(args[0]));
        return nullptr;
    }

    DirectConfig config;

    try {
        config = cfg::Config(in).as<DirectConfig>();
    } catch (const Error& err) {
        fmt::print("Failed to read config file '{}':\n{}\n", config_path, err.what());
        return nullptr;
    }

    LOGGER.log("Using direct presenting backend");
    return std::make_unique<DirectDisplay>(dispatcher, config);
}

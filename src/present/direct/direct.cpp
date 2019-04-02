#include "present/direct/direct.h"
#include <fstream>
#include <stdexcept>
#include <fmt/format.h>
#include "core/Config.h"
#include "core/Logger.h"
#include "core/Error.h"
#include "present/direct/DirectConfig.h"
#include "present/Event.h"

std::unique_ptr<DirectDisplay> make_direct_display(Span<const char*> args, EventDispatcher& dispatcher) {
    if (args.empty()) {
        fmt::print("Error: Missing argument <config>\n");
        return nullptr;
    }

    auto in = std::ifstream(args[0]);
    if (!in) {
        fmt::print("Error: Failed to open config file '{}'\n", std::string_view(args[0]));
        return nullptr;
    }

    DirectConfig config;

    try {
        config = cfg::Config(in).as<DirectConfig>();
    } catch (const Error& err) {
        fmt::print("Failed to read config file '{}':\n{}\n", args[0], err.what());
        return nullptr;
    }

    LOGGER.log("Using direct presenting backend");
    return std::make_unique<DirectDisplay>(dispatcher, config);
}

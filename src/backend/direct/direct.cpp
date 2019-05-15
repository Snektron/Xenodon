#include "backend/direct/direct.h"
#include <fstream>
#include "core/Config.h"
#include "core/Logger.h"
#include "core/Error.h"
#include "backend/direct/DirectConfig.h"
#include "backend/Event.h"

std::unique_ptr<DirectDisplay> create_direct_display(EventDispatcher& dispatcher, std::filesystem::path config) {
    LOGGER.log("Using direct presenting backend");

    auto in = std::ifstream(config);
    if (!in) {
        throw Error("Failed to open config file '{}'", config.native());
    }

    DirectConfig parsed_config;

    try {
        parsed_config = cfg::Config(in).as<DirectConfig>();
    } catch (const Error& err) {
        throw Error("Failed to read config file '{}': {}", config.native(), err.what());
    }

    return std::make_unique<DirectDisplay>(dispatcher, parsed_config);
}
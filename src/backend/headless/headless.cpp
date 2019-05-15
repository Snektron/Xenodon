#include "backend/headless/headless.h"
#include "core/Error.h"
#include "core/Logger.h"
#include "core/Config.h"
#include "backend/Event.h"
#include "backend/headless/HeadlessConfig.h"

std::unique_ptr<HeadlessDisplay> create_headless_display(EventDispatcher& dispatcher, std::filesystem::path config, std::filesystem::path output) {
    LOGGER.log("Using headless presenting backend");

    auto in = std::ifstream(config);
    if (!in) {
        throw Error("Failed to open config file '{}'", config.native());
    }

    HeadlessConfig parsed_config;

    try {
        parsed_config = cfg::Config(in).as<HeadlessConfig>();
    } catch (const Error& err) {
        throw Error("Failed to read config file '{}': {}", config.native(), err.what());
    }

    return std::make_unique<HeadlessDisplay>(dispatcher, parsed_config, output);
}

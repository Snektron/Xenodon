#include "backend/headless/headless.h"
#include <string_view>
#include <fmt/format.h>
#include "core/Error.h"
#include "core/Logger.h"
#include "core/Config.h"
#include "core/arg_parse.h"
#include "backend/Event.h"
#include "backend/headless/HeadlessConfig.h"

std::unique_ptr<HeadlessDisplay> make_headless_display(Span<const char*> args, EventDispatcher& dispatcher) {
    const char* config_path = nullptr;
    const char* output_path = "out.png";
    auto cmd = args::Command {
        .parameters = {
            {args::string_opt(&output_path), "output image", "--output", 'o'}
        },
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
        fmt::print("Error: Failed to open config file '{}'\n", config_path);
        return nullptr;
    }

    HeadlessConfig config;

    try {
        config = cfg::Config(in).as<HeadlessConfig>();
    } catch (const Error& err) {
        fmt::print("Failed to read config file '{}':\n{}\n", config_path, err.what());
        return nullptr;
    }

    LOGGER.log("Using headless presenting backend");

    return std::make_unique<HeadlessDisplay>(dispatcher, config, output_path);
}

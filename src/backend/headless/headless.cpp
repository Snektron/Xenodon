#include "backend/headless/headless.h"
#include <string_view>
#include <fmt/format.h>
#include "core/Error.h"
#include "core/Logger.h"
#include "core/Config.h"
#include "backend/Event.h"
#include "backend/headless/HeadlessConfig.h"

std::unique_ptr<HeadlessDisplay> make_headless_display(Span<const char*> args, EventDispatcher& dispatcher) {
    size_t i = 0;
    const char* output = "out.png";

    for (; i < args.size(); ++i) {
        auto arg = std::string_view(args[i]);
        if (arg == "-o" || arg == "--output") {
            if (++i == args.size()) {
                fmt::print("Error: {} expects argument <output image>\n", arg);
                return nullptr;
            } else {
                output = args[i];
            }
        } else {
            break;
        }
    }

    if (i == args.size()) {
        fmt::print("Error: expected argument <config>\n");
        return nullptr;
    }

    auto in = std::ifstream(args[i]);
    if (!in) {
        fmt::print("Error: Failed to open config file '{}'\n", std::string_view(args[0]));
        return nullptr;
    }

    HeadlessConfig config;

    try {
        config = cfg::Config(in).as<HeadlessConfig>();
    } catch (const Error& err) {
        fmt::print("Failed to read config file '{}':\n{}\n", args[0], err.what());
        return nullptr;
    }

    LOGGER.log("Using headless presenting backend");

    return std::make_unique<HeadlessDisplay>(dispatcher, config, output);
}

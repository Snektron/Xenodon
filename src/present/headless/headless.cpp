#include "present/headless/headless.h"
#include <string_view>
#include <fmt/format.h>
#include "core/Logger.h"
#include "core/Config.h"
#include "present/Event.h"
#include "present/headless/HeadlessConfig.h"

std::unique_ptr<HeadlessDisplay> make_headless_display(Span<const char*> args, EventDispatcher& dispatcher) {
    size_t i = 0;
    const char* output = "out.png";

    for (; i < args.size(); ++i) {
        auto arg = std::string_view(args[0]);
        if (arg == "-o" || arg == "--output") {
            if (++i == args.size()) {
                fmt::print("Error: expected argument <output image>\n");
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

    auto in = std::ifstream(args[0]);
    if (!in) {
        fmt::print("Error: Failed to open config file '{}'\n", std::string_view(args[0]));
        return nullptr;
    }

    HeadlessConfig config;

    try {
        config = cfg::Config(in).as<HeadlessConfig>();
    } catch (const std::runtime_error& err) {
        fmt::print("Failed to read config file '{}':\n{}\n", args[0], err.what());
        return nullptr;
    }

    LOGGER.log("Using headless presenting backend");

    return std::make_unique<HeadlessDisplay>(dispatcher, config, output);
}

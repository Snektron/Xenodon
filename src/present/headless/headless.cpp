#include "present/headless/headless.h"
#include <vector>
#include <string_view>
#include <algorithm>
#include <fmt/format.h>
#include "present/Event.h"
#include "core/Logger.h"
#include "core/Parser.h"

template<>
struct parser::Parse<vk::Extent2D> {
    vk::Extent2D operator()(parser::Parser& p) const {
        size_t x = parser::Parse<size_t>{}(p);
        p.expect('x');
        size_t y = parser::Parse<size_t>{}(p);

        return {
            static_cast<uint32_t>(x),
            static_cast<uint32_t>(y)
        };
    }
};

template<>
struct parser::Parse<std::vector<size_t>> {
    std::vector<size_t> operator()(parser::Parser& p) const {
        auto gpus = std::vector<size_t>();
        gpus.push_back(parser::Parse<size_t>{}(p));
        while (p.peek() == ',') {
            p.consume();

            auto gpu = parser::Parse<size_t>{}(p);

            if (std::find(gpus.begin(), gpus.end(), gpu) != gpus.end()) {
                throw ParseError(p, "Duplicate gpu {}", gpu);
            }

            gpus.push_back(gpu);
        }

        return gpus;
    }
};

std::unique_ptr<HeadlessDisplay> make_headless_display(Span<const char*> args, EventDispatcher& dispatcher) {
    std::vector<size_t> gpu_whitelist;

    size_t i = 0;
    for (; i < args.size(); ++i) {
        auto arg = std::string_view(args[i]);

        if (arg == "--gpus") {
            if (++i == args.size()) {
                fmt::print("Error: --gpus expects argument <gpu list>\n");
                return nullptr;                
            }

            try {
                gpu_whitelist = parser::parse<std::vector<size_t>>(std::string_view(args[i]));
            } catch (const parser::ParseError& err) {
                fmt::print("Failed to parse --gpus argument:\n{}\n", err.what());
                return nullptr;
            }
        } else {
            break;
        }
    }

    if (i == args.size()) {
        fmt::print("Error: expected argument <extent>\n");
        return 0;
    }

    vk::Extent2D extent;

    try {
        extent = parser::parse<vk::Extent2D>(args[i]);
    } catch (const parser::ParseError& err) {
        fmt::print("Failed to parse <extent> argument:\n{}\n", err.what());
        return nullptr;
    }

    return nullptr;
}

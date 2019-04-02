#include <string_view>
#include <array>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include <fmt/format.h>
#include "resources.h"
#include "version.h"
#include "main_loop.h"
#include "sysinfo.h"
#include "core/Logger.h"
#include "backend/Display.h"
#include "backend/Event.h"
#include "backend/headless/headless.h"
#include "utility/Span.h"

#if defined(XENODON_PRESENT_XORG)
    #include "backend/xorg/xorg.h"
#endif

#if defined(XENODON_PRESENT_DIRECT)
    #include "backend/direct/direct.h"
#endif

namespace {
    void print_help(const char* program_name) {
        fmt::print(resources::open("resources/help.txt"), program_name);
    }

    std::unique_ptr<Display> make_display(Span<const char*> args, EventDispatcher& dispatcher) {
        auto backend = std::string_view(args[0]);
        if (backend == "headless") {
            return make_headless_display(args.sub(1), dispatcher);
        } else if (backend == "xorg") {
            #if defined(XENODON_PRESENT_XORG)
                return make_xorg_display(args.sub(1), dispatcher);
            #else
                fmt::print("Error: Xorg backend was disabled\n");
                return nullptr;
            #endif
        } else if (backend == "direct") {
            #if defined(XENODON_PRESENT_DIRECT)
                return make_direct_display(args.sub(1), dispatcher);
            #else
                fmt::print("Error: Direct backend was disabled\n");
                return nullptr;
            #endif
        } else {
            fmt::print("Error: no such presenting backend '{}'", backend);
            return nullptr;
        }
    }

    void render(Span<const char*> args) {
        bool quiet = false;
        const char* log_output = nullptr;

        size_t i = 0;
        for (; i < args.size(); ++i) {
            auto arg = std::string_view(args[i]);

            if (arg.size() == 0 || arg[0] != '-') {
                break;
            } else if (arg == "-q" || arg == "--quiet") {
                quiet = true;
            } else if (arg == "--log") {
                if (++i == args.size()) {
                    fmt::print("Error: --log expects argument <file>\n");
                    return;
                }

                log_output = args[i];
            }
        }

        if (i == args.size()) {
            fmt::print("Error: expected argument <present backend>\n");
            return;
        }

        if (!quiet) {
            LOGGER.add_sink<ConsoleSink>();
        }

        if (log_output) {
            LOGGER.add_sink<FileSink>(log_output);
        }

        auto dispatcher = EventDispatcher();
        auto display = make_display(args.sub(i), dispatcher);

        if (!display) {
            return;
        }

        main_loop(dispatcher, display.get());
    }
}

int main(int argc, const char* argv[]) {
    if (argc <= 1) {
        fmt::print("Error: Subcommand required, see `{} help`\n", argv[0]);
        return 0;
    }

    auto args = Span<const char*>(static_cast<size_t>(argc - 2), &argv[2]);
    auto subcommand = std::string_view(argv[1]);

    if (subcommand == "help") {
        print_help(argv[0]);
    } else if (subcommand == "sysinfo") {
        sysinfo();
    } else if (subcommand == "render") {
        render(args);
    } else {
        fmt::print("Error: Invalid subcommand '{}', see `{} help`\n", subcommand, argv[0]);
    }

    return 0;
}

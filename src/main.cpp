#include <string_view>
#include <array>
#include <filesystem>
#include <cstddef>
#include <cstdlib>
#include <vulkan/vulkan.hpp>
#include <fmt/format.h>
#include "core/Logger.h"
#include "core/Error.h"
#include "core/arg_parse.h"
#include "backend/backend.h"
#include "backend/Display.h"
#include "backend/Event.h"
#include "utility/Span.h"
#include "resources.h"
#include "main_loop.h"
#include "sysinfo.h"
#include "convert.h"

namespace {
    void print_help(const char* program_name) {
        fmt::print(resources::open("resources/help.txt"), program_name);
    }

    struct RenderOptions {
        bool quiet = false;
        std::filesystem::path log_output;

        RenderParameters render_params;

        struct {
            std::filesystem::path config;
            std::filesystem::path output;

            bool enabled() const {
                return !this->config.empty();
            }
        } headless;

        struct {
             std::filesystem::path config;

             bool enabled() const {
                return !this->config.empty();
            }
        } direct;

        struct {
            bool enabled = false;
            std::filesystem::path multi_gpu_config;
        } xorg;
    };

    RenderOptions parse_render_args(Span<const char*> args) {
        RenderOptions opts;

        auto cmd = args::Command {
            .flags = {
                {&opts.quiet, "--quiet", 'q'},
                {&opts.xorg.enabled, "--xorg"}
            },
            .parameters = {
                {args::path_opt(&opts.log_output), "output path", "--log-output"},
                {args::path_opt(&opts.headless.config), "config path", "--headless"},
                {args::path_opt(&opts.headless.output), "output path", "--output"},
                {args::path_opt(&opts.direct.config), "config path", "--direct"},
                {args::path_opt(&opts.xorg.multi_gpu_config), "config path", "--xorg-multi-gpu"},
                {args::float_range_opt(&opts.render_params.density, 0.f), "density", "--density"},
                {args::string_opt(&opts.render_params.model_type_override), "model type", "--model-type"}
            },
            .positional = {
                {args::path_opt(&opts.render_params.model_path), "model"}
            }
        };

        args::parse(args, cmd);

        int enabled_backends =
            static_cast<int>(opts.xorg.enabled) +
            static_cast<int>(opts.headless.enabled()) +
            static_cast<int>(opts.direct.enabled());

        if (enabled_backends == 0) {
            throw Error("Missing required backend --xorg, --headless or --direct");
        } else if (enabled_backends > 1) {
            throw Error("--xorg, --headless and --direct are mutually exclusive");
        }

        if (!opts.headless.output.empty() && !opts.headless.enabled()) {
            throw Error("--output requires --headless");
        } else if (opts.headless.output.empty()) {
            opts.headless.output = "out.png";
        }

        if (!opts.xorg.multi_gpu_config.empty() && !opts.xorg.enabled) {
            throw Error("--xorg-multi-gpu requires --xorg");
        }

        return opts;
    }

    void render(Span<const char*> args) {
        RenderOptions opts;
        try {
            opts = parse_render_args(args);
        } catch (const Error& e) {
            fmt::print("Error: {}\n", e.what());
            return;
        }

        if (!opts.quiet) {
            LOGGER.add_sink<ConsoleSink>();
        }

        if (!opts.log_output.empty()) {
            LOGGER.add_sink<FileSink>(opts.log_output);
        }

        auto dispatcher = EventDispatcher();
        std::unique_ptr<Display> display;

        try {
            if (opts.xorg.enabled) {
                display = create_xorg_backend(dispatcher, opts.xorg.multi_gpu_config);
            } else if (opts.direct.enabled()) {
                display = create_direct_backend(dispatcher, opts.direct.config);
            } else {
                display = create_headless_backend(dispatcher, opts.headless.config, opts.headless.output);
            }
        } catch (const Error& e) {
            fmt::print("Error: Failed to initialize backend: {}", e.what());
            return;
        }

        try {
            main_loop(dispatcher, display.get(), opts.render_params);
        } catch (const Error& e) {
            fmt::print("Error: {}\n", e.what());
        }
    }
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
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
    } else if (subcommand == "convert") {
        convert(args);
    } else {
        fmt::print("Error: Invalid subcommand '{}', see `{} help`\n", subcommand, argv[0]);
    }

    return EXIT_SUCCESS;
}

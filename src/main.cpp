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
    struct HelpTopic {
        std::string_view topic;
        std::string_view help_message;
    };

    constexpr const auto HELP_TOPICS = std::array {
        HelpTopic{"help", resources::open("resources/help/help.txt")},
        HelpTopic{"sysinfo", resources::open("resources/help/sysinfo.txt")},
        HelpTopic{"convert", resources::open("resources/help/convert.txt")},
        HelpTopic{"render", resources::open("resources/help/render.txt")},
        HelpTopic{"xorg-multi-gpu", resources::open("resources/help/xorg_multi_gpu.txt")},
        HelpTopic{"headless-config", resources::open("resources/help/headless_config.txt")},
        HelpTopic{"direct-config", resources::open("resources/help/direct_config.txt")},
    };

    struct RenderOptions {
        bool quiet = false;
        std::filesystem::path log_output;
        RenderParameters render_params;

        struct {
            std::filesystem::path config;
            std::string_view output;
            bool discard_output = false;

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

    auto voxel_ratio_opt(Vec3F* var) {
        return [var](std::string_view arg) {
            const auto first = arg.find(':');
            const auto second = arg.find(':', first + 1);

            if (first == std::string_view::npos || second == std::string_view::npos) {
                return false;
            }

            const std::string_view x = arg.substr(0, first);
            const std::string_view y = arg.substr(first + 1, second - first - 1);
            const std::string_view z = arg.substr(second + 1);

            if (x.empty() || y.empty() || z.empty()) {
                return false;
            }

            Vec3F value;
            bool valid = true;
            valid &= args::parse_float(x, value.x);
            valid &= args::parse_float(y, value.y);
            valid &= args::parse_float(z, value.z);

            if (valid && value.x > 0 && value.y > 0 && value.z > 0) {
                *var = value;
                return true;
            }

            return false;
        };
    }

    RenderOptions parse_render_args(Span<const char*> args) {
        RenderOptions opts;

        auto cmd = args::Command {
            .flags = {
                {&opts.quiet, "--quiet", 'q'},
                {&opts.xorg.enabled, "--xorg"},
                {&opts.headless.discard_output, "--discard-output"}
            },
            .parameters = {
                {args::path_opt(&opts.log_output), "output path", "--log-output"},
                {args::path_opt(&opts.headless.config), "config path", "--headless"},
                {args::string_opt(&opts.headless.output), "output path", "--output"},
                {args::path_opt(&opts.direct.config), "config path", "--direct"},
                {args::path_opt(&opts.xorg.multi_gpu_config), "config path", "--xorg-multi-gpu"},
                {args::float_range_opt(&opts.render_params.emission_coeff, 0.f), "emission coefficient", "--emission-coeff", 'e'},
                {args::string_opt(&opts.render_params.volume_type_override), "volume type", "--volume-type"},
                {args::string_opt(&opts.render_params.shader), "shader", "--shader", 's'},
                {voxel_ratio_opt(&opts.render_params.voxel_ratio), "voxel dimension ratio", "--voxel-ratio", 'r'},
                {args::path_opt(&opts.render_params.stats_save_path), "stats output", "--stats-output"},
                {args::string_opt(&opts.render_params.camera), "camera", "--camera"},
                {args::int_range_opt(&opts.render_params.repeat), "frame repeat", "--repeat"}
            },
            .positional = {
                {args::path_opt(&opts.render_params.volume_path), "volume path"}
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

        if (!opts.headless.enabled() && opts.headless.discard_output) {
            throw Error("--dont-save requires --headless");
        }

        if (!opts.headless.output.empty() && !opts.headless.enabled()) {
            throw Error("--output requires --headless");
        } else if (opts.headless.output.empty()) {
            opts.headless.output = "out-{}.png";
        } else if (opts.headless.discard_output) {
            throw Error("--dont-save and --output are mutually exclusive");
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
                std::string_view output = opts.headless.discard_output ? "" : opts.headless.output;
                display = create_headless_backend(opts.headless.config, output);
            }
        } catch (const Error& e) {
            fmt::print("Error: Failed to initialize backend: {}\n", e.what());
            return;
        }

        try {
            main_loop(dispatcher, display.get(), opts.render_params);
        } catch (const Error& e) {
            fmt::print("Error: {}\n", e.what());
        }
    }

    void help(const char* program_name, Span<const char*> args) {
        if (args.empty()) {
            fmt::print("{}", resources::open("resources/help.txt"));
            return;
        } else if (args.size() != 1) {
            fmt::print("Error: Invalid usage of subcommand 'help', see '{} help'\n", program_name);
            return;
        }

        auto topic = std::string_view(args.front());

        auto it = std::find_if(
            HELP_TOPICS.begin(),
            HELP_TOPICS.end(),
            [&topic](const HelpTopic& help) {
            return help.topic == topic;
        });

        if (it != HELP_TOPICS.end()) {
            fmt::print("{}", it->help_message);
        } else {
            fmt::print("Error: No such topic {}, see '{} help'\n", topic, program_name);
        }
    }
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        help(argv[0], nullptr);
        return 0;
    }

    auto args = Span<const char*>(static_cast<size_t>(argc - 2), &argv[2]);
    auto subcommand = std::string_view(argv[1]);

    if (subcommand == "help") {
        help(argv[0], args);
    } else if (subcommand == "sysinfo") {
        sysinfo();
    } else if (subcommand == "render") {
        render(args);
    } else if (subcommand == "convert") {
        convert(args);
    } else {
        fmt::print("Error: Invalid subcommand '{}', see '{} help'\n", subcommand, argv[0]);
    }

    return EXIT_SUCCESS;
}

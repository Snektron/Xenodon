#include <string_view>
#include <array>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include <fmt/format.h>
#include "present/Display.h"
#include "present/Event.h"
#include "core/Logger.h"
#include "utility/Span.h"
#include "resources.h"
#include "version.h"
#include "main_loop.h"
#include "present/headless/headless.h"

#if defined(XENODON_PRESENT_XORG)
    #include "present/xorg/xorg.h"
#endif

#if defined(XENODON_PRESENT_DIRECT)
    #include "present/direct/direct.h"
#endif

namespace {
    void print_help(const char* program_name) {
        fmt::print(resources::open("resources/help.txt"), program_name);
    }

    void subcommand_sysinfo() {
        constexpr const std::array required_instance_extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_DISPLAY_EXTENSION_NAME
        };

        auto instance = vk::createInstanceUnique(
            vk::InstanceCreateInfo(
                {},
                &version::APP_INFO,
                0,
                nullptr,
                required_instance_extensions.size(),
                required_instance_extensions.data()
            )
        );

        auto getPhysicalDeviceProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
            instance->getProcAddr("vkGetPhysicalDeviceProperties2")
        );

        fmt::print("System setup information\n");
        auto gpus = instance->enumeratePhysicalDevices();
        if (gpus.empty()) {
            fmt::print("No GPUs detected\n");
            return;
        }

        for (size_t i = 0; i < gpus.size(); ++i) {
            fmt::print("GPU {}:\n", i);

            auto pci_info = vk::PhysicalDevicePCIBusInfoPropertiesEXT();
            auto props2 = vk::PhysicalDeviceProperties2();

            const auto extensions = gpus[i].enumerateDeviceExtensionProperties();
            for (const auto& extension : extensions) {
                if (std::string_view(VK_EXT_PCI_BUS_INFO_EXTENSION_NAME) == extension.extensionName) {
                    props2.pNext = static_cast<void*>(&pci_info);
                    break;
                }
            }

            getPhysicalDeviceProperties2(
                static_cast<VkPhysicalDevice>(gpus[i]),
                reinterpret_cast<VkPhysicalDeviceProperties2*>(&props2)
            );

            fmt::print(
                "\tname: '{}'\n\ttype: {}\n",
                props2.properties.deviceName,
                vk::to_string(props2.properties.deviceType)
            );

            if (props2.pNext) {
                fmt::print(
                    "\tpci bus: {:0>8}:{:0>2}:{:0>2}.{}\n",
                    pci_info.pciDomain,
                    pci_info.pciBus,
                    pci_info.pciDevice,
                    pci_info.pciFunction);
            } else {
                fmt::print("\tskipping bus info (extension not present)\n");
            }

            auto display_props = gpus[i].getDisplayPropertiesKHR();

            if (display_props.empty()) {
                fmt::print("\tNo displays detected\n");
                continue;
            }

            for (size_t j = 0; j < display_props.size(); ++j) {
                fmt::print("\tDisplay {}:\n\t\tname: ", j);

                if (display_props[j].displayName) {
                    fmt::print("'{}'", display_props[j].displayName);
                } else {
                    fmt::print("(null)");
                }

                auto res = display_props[j].physicalResolution;
                fmt::print("\n\t\tresolution: {}x{}\n", res.width, res.height);
            }
        }
    }

    std::unique_ptr<Display> make_display(Span<const char*> args, EventDispatcher& dispatcher) {
        auto backend = std::string_view(args[0]);
        if (backend == "headless") {
            return make_headless_display(args.sub(1), dispatcher);
        } else if (backend == "xorg") {
            #if defined(XENODON_PRESENT_XORG)
                return make_xorg_display(args.sub(1), dispatcher);
            #else
                fmt::print("Error: Xorg support was disabled\n");
                return nullptr;
            #endif
        } else if (backend == "direct") {
            #if defined(XENODON_PRESENT_DIRECT)
                return make_direct_display(args.sub(1), dispatcher);
            #else
                fmt::print("Error: Direct support was disabled\n");
                return nullptr;
            #endif
        } else {
            fmt::print("Error: no such presenting backend '{}'", backend);
            return nullptr;
        }
    }

    void subcommand_render(Span<const char*> args) {
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
        subcommand_sysinfo();
    } else if (subcommand == "render") {
        subcommand_render(args);
    } else {
        fmt::print("Error: Invalid subcommand '{}', see `{} help`\n", subcommand, argv[0]);
    }

    return 0;
}

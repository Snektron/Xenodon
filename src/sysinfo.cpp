#include "sysinfo.h"
#include <vulkan/vulkan.hpp>
#include <fmt/format.h>
#include "graphics/core/Instance.h"
#include "graphics/core/PhysicalDevice.h"
#include "version.h"

void sysinfo() {
    constexpr const std::array required_instance_extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_DISPLAY_EXTENSION_NAME
    };

    auto instance = Instance(required_instance_extensions);

    // For some reason this symbol is not exported from libvulkan
    auto getPhysicalDeviceProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
        instance->getProcAddr("vkGetPhysicalDeviceProperties2")
    );

    fmt::print("System setup information\n");
    const auto& gpus = instance.physical_devices();
    if (gpus.empty()) {
        fmt::print("No GPUs detected\n");
        return;
    }

    for (size_t i = 0; i < gpus.size(); ++i) {
        const auto& gpu = gpus[i];

        auto pci_info = vk::PhysicalDevicePCIBusInfoPropertiesEXT();
        auto props2 = vk::PhysicalDeviceProperties2();

        if (gpu.supports_extensions(VK_EXT_PCI_BUS_INFO_EXTENSION_NAME)) {
            props2.pNext = static_cast<void*>(&pci_info);
        }

        getPhysicalDeviceProperties2(
            static_cast<VkPhysicalDevice>(gpu.get()),
            reinterpret_cast<VkPhysicalDeviceProperties2*>(&props2)
        );

        fmt::print(
            "GPU {}:\n"
            "\tname: '{}'\n"
            "\ttype: {}\n",
            i,
            props2.properties.deviceName,
            vk::to_string(props2.properties.deviceType)
        );

        if (props2.pNext) {
            fmt::print(
                "\tpci bus: {:0>8}:{:0>2}:{:0>2}.{}\n",
                pci_info.pciDomain,
                pci_info.pciBus,
                pci_info.pciDevice,
                pci_info.pciFunction
            );
        }

        auto display_props = gpu->getDisplayPropertiesKHR();

        if (display_props.empty()) {
            fmt::print("\tNo displays detected\n");
            continue;
        }

        for (size_t j = 0; j < display_props.size(); ++j) {
            fmt::print("\tDisplay {}:\n", j);

            if (display_props[j].displayName) {
                fmt::print("\t\tname: '{}'\n", display_props[j].displayName);
            }

            auto res = display_props[j].physicalResolution;
            fmt::print("\t\tresolution: {}x{}\n", res.width, res.height);
        }
    }
}

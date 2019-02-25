#include "interactive/interactive.h"
#include <iostream>
#include <array>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <limits>
#include <string_view>
#include <chrono>
#include <vector>
#include <memory>
#include <cstring>
#include <cstdlib>
#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include "render/DeviceContext.h"
#include "render/PhysicalDeviceInfo.h"
#include "interactive/Window.h"
#include "interactive/Swapchain.h"
#include "interactive/SurfaceInfo.h"
#include "interactive/Display.h"
#include "interactive/DisplayArray.h"
#include "utility/ScopeGuard.h"
#include "resources.h"

namespace {
    constexpr const std::array INSTANCE_EXTENSIONS = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME
    };

    constexpr const std::array DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct GpuInfo {
        vk::PhysicalDevice physical_device;
        uint32_t graphics_queue_index;
        uint32_t present_queue_index;
        uint32_t index;
    };

    GpuInfo pick_physical_device(vk::Instance instance, vk::SurfaceKHR surface) {
        auto physical_devices = instance.enumeratePhysicalDevices();

        auto graphics_supported = std::vector<uint32_t>();
        auto present_supported = std::vector<uint32_t>();
        for (size_t i = 0; i < physical_devices.size(); ++i) {
            const auto& device = physical_devices[i];

            auto info = PhysicalDeviceInfo(device);

            if (!info.supports_extensions(DEVICE_EXTENSIONS)
                || !info.supports_surface(surface)
                || (device.getProperties().deviceType != vk::PhysicalDeviceType::eDiscreteGpu
                    && device.getProperties().deviceType != vk::PhysicalDeviceType::eIntegratedGpu))
                continue;

            graphics_supported.clear();
            present_supported.clear();
            auto queue_families = device.getQueueFamilyProperties();

            uint32_t num_queues = static_cast<uint32_t>(queue_families.size());
            for (uint32_t i = 0; i < num_queues; ++i) {
                if (queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics)
                    graphics_supported.push_back(i);

                if (device.getSurfaceSupportKHR(i, surface))
                    present_supported.push_back(i);
            }

            if (graphics_supported.empty() || present_supported.empty())
                continue;

            auto git = graphics_supported.begin();
            auto pit = present_supported.begin();

            // Check if theres a queue with both supported
            while (git != graphics_supported.end() && pit != present_supported.end()) {
                uint32_t g = *git;
                uint32_t p = *pit;

                if (g == p)
                    return {device, g, p, static_cast<uint32_t>(i)};
                else if (g < p)
                    ++git;
                else
                    ++pit;
            }

            // No queue with both supported, but both are supported in any queue, so just take the first of both
            return {device, graphics_supported[0], present_supported[0], static_cast<uint32_t>(i)};
        }

        throw std::runtime_error("Failed to find a suitable physical device");
    }

    std::vector<std::unique_ptr<Display>> initialize_displays(vk::Instance instance, WindowContext& window_context) {
        xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(window_context.connection));
        auto displays = std::vector<std::unique_ptr<Display>>();
        displays.reserve(static_cast<size_t>(it.rem));

        for (int i = 0; it.rem; ++i, xcb_screen_next(&it)) {
            auto window = Window(window_context, it.data, 800, 600);
            auto surface = instance.createXcbSurfaceKHRUnique(window.surface_create_info());
            auto picked = pick_physical_device(instance, surface.get());

            std::cout << "Screen " << i << ": device " << picked.index << " (" << picked.physical_device.getProperties().deviceName << ')' << std::endl;
            auto device_context = DeviceContext(picked.physical_device, DEVICE_EXTENSIONS, picked.graphics_queue_index, picked.present_queue_index);

            auto area = vk::Rect2D({0, 0}, window.geometry().extent);

            displays.push_back(std::make_unique<Display>(std::move(device_context), std::move(window), std::move(surface), area));
        }

        return displays;
    }
}

void interactive_main(const vk::ApplicationInfo& app_info) {
    using namespace std::literals::chrono_literals;

    auto instance = vk::createInstanceUnique(
        vk::InstanceCreateInfo(
            {},
            &app_info,
            0,
            nullptr,
            INSTANCE_EXTENSIONS.size(),
            INSTANCE_EXTENSIONS.data()
        )
    );

    auto window_context = WindowContext();
    auto display_array = DisplayArray(window_context, initialize_displays(instance.get(), window_context));

    auto start = std::chrono::high_resolution_clock::now();
    size_t start_frame = 0;
    size_t total_frames = 0;

    while (true) {
        while (auto event = window_context.poll_event()) {
            display_array.event(*event.get());
        }

        if (display_array.close_requested)
            break;

        total_frames++;

        display_array.present();

        auto now = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration<double>(now - start);

        if (diff > 1s) {
            size_t frames = total_frames - start_frame;
            std::cout << "FPS: " << static_cast<double>(frames) / diff.count() << std::endl;
            start_frame = total_frames;
            start = now;
        }
    }
}

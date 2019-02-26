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
#include <X11/keysym.h>
#include "render/DeviceContext.h"
#include "render/PhysicalDeviceInfo.h"
#include "interactive/Window.h"
#include "interactive/Swapchain.h"
#include "interactive/SurfaceInfo.h"
#include "interactive/Display.h"
#include "interactive/DisplayArray.h"
#include "interactive/EventLoop.h"
#include "utility/ScopeGuard.h"
#include "utility/bind_member.h"
#include "resources.h"

namespace {
    constexpr const std::array INSTANCE_EXTENSIONS = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
        VK_KHR_DISPLAY_EXTENSION_NAME
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

    void dump_gpu_info(vk::Instance instance) {
        auto display_name = [](auto& props) {
            if (props.displayName)
                return props.displayName;
            else
                return "(null)";
        };

        auto physical_devices = instance.enumeratePhysicalDevices();
        for (size_t i = 0; i < physical_devices.size(); ++i) {
            std::cout << "GPU " << i << ":\n";
            auto& gpu = physical_devices[i];
            auto props = gpu.getProperties();
            std::cout << "\tDevice name: " << props.deviceName << "\n";
            auto display_props = gpu.getDisplayPropertiesKHR();
            size_t skipped = 0;
            for (size_t j = 0; j < display_props.size(); ++j) {
                uint32_t w = display_props[j].physicalResolution.width;
                uint32_t h = display_props[j].physicalResolution.height;

                if (w == 0 && h == 0) {
                    ++skipped;
                    continue;
                }

                std::cout << "\tDisplay " << j << ":\n";
                std::cout << "\t\tDisplay name: " << display_name(display_props[j]) << '\n';
                std::cout << "\t\tResolution: " << w << "x" << h << '\n';
            }

            if (skipped > 0)
                std::cout << "\t(Skipped " << skipped << " displays)\n";
        }

        std::cout << std::endl;
    }

    vk::UniqueSurfaceKHR create_display_surface(vk::Instance instance, vk::PhysicalDevice gpu) {
        uint32_t count = 1;
        vk::DisplayPropertiesKHR display_props;
        auto res = gpu.getDisplayPropertiesKHR(&count, &display_props);
        if ((res != vk::Result::eSuccess && res != vk::Result::eIncomplete) || count != 1) {
            std::cout << vk::to_string(res) << " " << count << std::endl;
            throw std::runtime_error("Failed to get display properties");
        }

        // auto display_props = gpu.getDisplayPropertiesKHR().front();
        auto display = display_props.display;
        auto mode_props = gpu.getDisplayModePropertiesKHR(display).front();
        auto plane_props = gpu.getDisplayPlanePropertiesKHR();

        uint32_t plane;
        bool found = false;
        for (plane = 0; plane < plane_props.size(); ++plane) {
            if (plane_props[plane].currentDisplay != vk::DisplayKHR(nullptr) && plane_props[plane].currentDisplay != display)
                continue;

            auto supported_displays = gpu.getDisplayPlaneSupportedDisplaysKHR(plane);
            if (supported_displays.size() == 0)
                continue;

            for (uint32_t i = 0; i < supported_displays.size(); ++i) {
                if (supported_displays[i] == display) {
                    found = true;
                }
            }

            if (found)
                break;
        }

        if (!found)
            throw std::runtime_error("Failed to find compatible plane");

        auto create_info = vk::DisplaySurfaceCreateInfoKHR();
        create_info.flags = {};
        create_info.displayMode = mode_props.displayMode;
        create_info.planeIndex = plane;
        create_info.planeStackIndex = plane_props[plane].currentStackIndex;
        create_info.transform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        create_info.alphaMode = vk::DisplayPlaneAlphaFlagBitsKHR::eOpaque;
        create_info.globalAlpha = 1.0f;
        create_info.imageExtent = mode_props.parameters.visibleRegion;

        return instance.createDisplayPlaneSurfaceKHRUnique(create_info);
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

    dump_gpu_info(instance.get());

    create_display_surface(instance.get(), instance->enumeratePhysicalDevices()[0]);

    auto window_context = WindowContext();
    auto event_loop = EventLoop(window_context);
    auto display_array = DisplayArray(initialize_displays(instance.get(), window_context));

    bool run = true;
    auto quit = [&run] {
        run = false;
    };

    event_loop.bind(XK_Escape, std::bind(quit));
    event_loop.bind_quit(quit);
    event_loop.bind_reconfigure(bind_member(display_array, &DisplayArray::reconfigure));

    auto start = std::chrono::high_resolution_clock::now();
    size_t frames = 0;

    while (run) {
        ++frames;

        display_array.present();

        auto now = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration<double>(now - start);

        if (diff > 1s) {
            std::cout << "FPS: " << static_cast<double>(frames) / diff.count() << std::endl;
            frames = 0;
            start = now;
        }

        event_loop.poll_events();
    }
}

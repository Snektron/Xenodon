#include <iostream>
#include <array>
#include <stdexcept>
#include <optional>
#include <utility>
#include <algorithm>
#include <cstdlib>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include "utility/ScopeGuard.h"

namespace {
    constexpr const char* const APP_NAME = "Xenodon";
    constexpr const uint32_t APP_VERSION = VK_MAKE_VERSION(0, 0, 0);
}

vk::UniqueInstance create_instance() {
    auto app_info = vk::ApplicationInfo(
        APP_NAME,
        APP_VERSION,
        nullptr,
        0,
        VK_API_VERSION_1_1
    );

    uint32_t ext_count;
    const auto** extensions = glfwGetRequiredInstanceExtensions(&ext_count);

    auto create_info = vk::InstanceCreateInfo(
        {},
        &app_info,
        0,
        nullptr,
        ext_count,
        extensions
    );

    return vk::createInstanceUnique(create_info);
}

std::tuple<vk::PhysicalDevice, uint32_t, uint32_t> pick_physical_device(vk::UniqueInstance& instance, vk::UniqueSurfaceKHR& surface) {
    auto physical_devices = instance->enumeratePhysicalDevices();

    auto graphics_supported = std::vector<size_t>();
    auto present_supported = std::vector<size_t>();

    for (auto&& device : physical_devices) {
        graphics_supported.clear();
        present_supported.clear();
        auto queue_families = device.getQueueFamilyProperties();

        for (size_t i = 0; i < queue_families.size(); ++i) {
            const auto& props = queue_families[i];
            if (props.queueFlags & vk::QueueFlagBits::eGraphics)
                graphics_supported.push_back(i);

            if (device.getSurfaceSupportKHR(i, surface.get()))
                present_supported.push_back(i);
        }

        if (graphics_supported.empty() || present_supported.empty())
            continue;

        auto git = graphics_supported.begin();
        auto pit = present_supported.begin();

        // Check if theres a queue with both supported
        while (git != graphics_supported.end() && pit != present_supported.end()) {
            size_t g = *git;
            size_t p = *pit;

            if (g == p)
                return {device, g, p};
            else if (g < p)
                ++git;
            else
                ++pit;
        }

        // No queue with both supported, but both are supported in any queue, so just take the first of both
        return {device, graphics_supported[0], present_supported[0]};
    }

    throw std::runtime_error("Failed to find a suitable physical device");
}

template <typename... QueueIndices>
vk::UniqueDevice initialize_device(const vk::PhysicalDevice& physical_device, QueueIndices&&... indices) {
    float priority = 1.0f;

    auto queue_create_infos = std::array{
        vk::DeviceQueueCreateInfo(
            {},
            indices,
            1,
            &priority
        )...
    };

    auto device_create_info = vk::DeviceCreateInfo(
        {},
        queue_create_infos.size(),
        queue_create_infos.data()
    );

    return physical_device.createDeviceUnique(device_create_info);
}

vk::UniqueSurfaceKHR create_surface(vk::UniqueInstance& instance, GLFWwindow* window) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(VkInstance(instance.get()), window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface");

    return vk::UniqueSurfaceKHR(vk::SurfaceKHR(surface));
}

int main() {
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    auto _finalize_glfw = ScopeGuard([] {
        glfwTerminate();
    });

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    auto* window = glfwCreateWindow(800, 600, "Vulkan test", nullptr, nullptr);

    auto _finalize_window = ScopeGuard([window] {
        glfwDestroyWindow(window);
    });

    auto instance = create_instance();
    auto surface = create_surface(instance, window);
    auto [physical_device, graphics_queue_index, present_queue_index] = pick_physical_device(instance, surface);
    std::cout << "Picked device '" << physical_device.getProperties().deviceName << "'\n";
    auto device = initialize_device(physical_device, graphics_queue_index, present_queue_index);
    auto graphics_queue = device->getQueue(graphics_queue_index, 0);
    auto present_queue = device->getQueue(present_queue_index, 0);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

#include <iostream>
#include <array>
#include <cstdlib>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include "graphics/debug_utility.h"
#include "utility/utility.h"
#include "math/Vec.h"

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

int main() {
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    Defer _finalize_glfw([] {
        glfwTerminate();
    });

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    auto* window = glfwCreateWindow(800, 600, "Vulkan test", nullptr, nullptr);

    Defer _finalize_window([window] {
        glfwDestroyWindow(window);
    });

    auto instance = create_instance();
    auto physical_devices = instance->enumeratePhysicalDevices();

    for (auto&& device : physical_devices) {
        auto props = device.getProperties();
        std::cout << 
            "Device: \n"
            "\tapi version: " << props.apiVersion << "\n"
            "\tdevice id: " << props.deviceID << "\n"
            "\tdevice type: " << props.deviceType << "\n\n";
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

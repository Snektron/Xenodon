#include <iostream>
#include <utility>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace {
    constexpr const char* APP_NAME = "Xenodon";
    constexpr const uint32_t APP_VERSION = VK_MAKE_VERSION(0, 0, 0);
}

template <typename F>
struct Defer {
    F f;

    Defer(F&& f):
        f(std::forward<F>(f)) {
    }

    ~Defer() {
        this->f();
    }
};

constexpr const char* fmt_device_type(vk::PhysicalDeviceType ty) {
    switch (ty) {
        case vk::PhysicalDeviceType::eOther:
            return "Other";
        case vk::PhysicalDeviceType::eIntegratedGpu:
            return "Integrated GPU";
        case vk::PhysicalDeviceType::eDiscreteGpu:
            return "Discrete GPU";
        case vk::PhysicalDeviceType::eVirtualGpu:
            return "Virtual GPU";
        case vk::PhysicalDeviceType::eCpu:
            return "CPU";
    }
}

int main() {
    // if (glfwInit() != GLFW_TRUE) {
    //     std::cerr << "Failed to initialize GLFW" << std::endl;
    //     return 1;
    // }

    // Defer _finalize_glfw([] {
    //     glfwTerminate();
    // });

    // glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // auto* window = glfwCreateWindow(800, 600, "Vulkan test", nullptr, nullptr);

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

    auto instance = vk::createInstanceUnique(create_info);

    auto physical_devices = instance->enumeratePhysicalDevices();

    for (auto&& device : physical_devices) {
        auto props = device.getProperties();
        std::cout << 
            "Device: \n"
            "\tapi version: " << props.apiVersion << "\n"
            "\tdevice id: " << props.deviceID << "\n"
            "\tdevice type: " << fmt_device_type(props.deviceType) << "\n\n";
    }

    // while (!glfwWindowShouldClose(window)) {
    //     glfwPollEvents();
    // }

    // glfwDestroyWindow(window);
}

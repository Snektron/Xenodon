#ifndef _XENODON_GRAPHICS_PHYSICALDEVICEPICKER_H
#define _XENODON_GRAPHICS_PHYSICALDEVICEPICKER_H

#include <vector>
#include <algorithm>
#include <functional>
#include <array>
#include <string_view>
#include <vulkan/vulkan.hpp>

class PhysicalDevicePicker {
    vk::Instance instance;
    std::vector<vk::PhysicalDevice> physical_devices;

public:
    PhysicalDevicePicker(vk::Instance instance);

    template <typename F>
    PhysicalDevicePicker& filter(F f);

    template <typename It>
    PhysicalDevicePicker& with_extensions(It first, It last);

    template <typename C>
    PhysicalDevicePicker& with_extensions(const C& container) {
        return this->with_extensions(container.begin(), container.end());
    }

    template <typename T>
    PhysicalDevicePicker& with_extension(const T& extension) {
        return this->with_extension(std::array{extension});
    }

    template <typename F>
    PhysicalDevicePicker& for_each(F f);

    PhysicalDevicePicker& with_surface_support(vk::SurfaceKHR surface);
};

template <typename F>
PhysicalDevicePicker& PhysicalDevicePicker::filter(F f) {
    auto it = std::remove_if(
        this->physical_devices.begin(),
        this->physical_devices.end(),
        std::not_fn(f)
    );
    this->physical_devices.erase(it);

    return *this;
}

template <typename It>
PhysicalDevicePicker& PhysicalDevicePicker::with_extensions(It first, It last) {
    return this->filter([&](auto device) {
        auto exts = device.enumerateDeviceExtensionProperties();

        for (auto&& ext : exts) {
            auto cmp_ext = [&](auto& name) {
                return name == ext.extensionName;
            };

            if (std::find_if(first, last, cmp_ext) == last)
                return false;
        }

        return true;
    });
}

template <typename F>
PhysicalDevicePicker& PhysicalDevicePicker::for_each(F f) {
    std::for_each(
        this->physical_devices.begin(),
        this->physical_devices.end(),
        f
    );
    return *this;
}

#endif

#include "render/PhysicalDeviceInfo.h"

PhysicalDeviceInfo::PhysicalDeviceInfo(vk::PhysicalDevice physical_device):
    physical_device(physical_device),
    queue_family_properties(physical_device.getQueueFamilyProperties()),
    extension_properties(physical_device.enumerateDeviceExtensionProperties()) {
}

bool PhysicalDeviceInfo::supports_surface(vk::SurfaceKHR surface) const {
    auto formats = this->physical_device.getSurfaceFormatsKHR(surface);
    auto present_modes = this->physical_device.getSurfacePresentModesKHR(surface);

    return !formats.empty() && !present_modes.empty();
}
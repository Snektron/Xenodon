#include "graphics/PhysicalDevicePicker.h"

PhysicalDevicePicker::PhysicalDevicePicker(vk::Instance instance):
    instance(instance),
    physical_devices(instance.enumeratePhysicalDevices()) {
}

PhysicalDevicePicker& PhysicalDevicePicker::with_surface_support(vk::SurfaceKHR surface) {
    return this->filter([surface](auto device){
        auto formats = device.getSurfaceFormatsKHR(surface);
        auto present_modes = device.getSurfacePresentModesKHR(surface);

        return !formats.empty() && !present_modes.empty();
    });
}
#ifndef _XENODON_RENDER_RENDERER_H
#define _XENODON_RENDER_RENDERER_H

#include <vulkan/vulkan.hpp>

class Renderer {
    vk::Instance instance;
    vk::Rect2D area;
    vk::PhysicalDevice physical_device;
    vk::Device device;
    vk::Queue compute_queue;
    vk::Queue graphics_queue;
};

#endif

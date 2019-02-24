#ifndef _XENODON_RENDER_RENDERWORKER_H
#define _XENODON_RENDER_RENDERWORKER_H

#include "DeviceContext.h"
#include <vulkan/vulkan.hpp>

class RenderWorker {
    DeviceContext& device;
    vk::Rect2D area; // The area of the final display this worker takes up

public:
    RenderWorker(DeviceContext& device, vk::Rect2D area);

    friend class RenderManager;
};

#endif

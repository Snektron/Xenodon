#ifndef _XENODON_RENDER_RENDERER_H
#define _XENODON_RENDER_RENDERER_H

#include <memory>
#include <vector>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"
#include "backend/Display.h"
#include "render/DeviceRenderer.h"

class Renderer {
    std::vector<std::unique_ptr<DeviceRenderer>> devices;

public:
    Renderer(Display* display);
    void render();
    void recreate(size_t device, size_t output);
};

#endif

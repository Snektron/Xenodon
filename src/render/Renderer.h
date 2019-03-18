#ifndef _XENODON_RENDER_RENDERER_H
#define _XENODON_RENDER_RENDERER_H

#include <cstddef>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"
#include "present/Display.h"
#include "render/DeviceRenderer.h"

class Renderer {
    Display* display;
    std::vector<DeviceRenderer> devices;

public:
    Renderer(Display* display);
    void render();
    void recreate(size_t gpu, size_t screen);
};

#endif

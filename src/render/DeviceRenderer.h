#ifndef _XENODON_RENDER_DEVICERENDERER_H
#define _XENODON_RENDER_DEVICERENDERER_H

#include <vector>
#include <cstddef>
#include <vulkan/vulkan.h>
#include "graphics/Device.h"
#include "present/Display.h"
#include "present/Screen.h"
#include "render/TestRenderer.h"

struct RenderOutput {
    Screen* screen;
    vk::Rect2D region;
    TestRenderer renderer;
    std::vector<vk::UniqueFramebuffer> framebuffers;

    RenderOutput(Device& device, Screen* screen);
    RenderOutput(RenderOutput&&) = default;
    RenderOutput& operator=(RenderOutput&&) = default;

    void render();
};

struct DeviceRenderer {
    Device& device;

    std::vector<RenderOutput> outputs;

    DeviceRenderer(Display* display, size_t gpu, size_t screens);
    void render();
    void recreate(size_t screen);
};

#endif

#ifndef _XENODON_RENDER_DEVICERENDERER_H
#define _XENODON_RENDER_DEVICERENDERER_H

#include <vector>
#include <cstddef>
#include <vulkan/vulkan.h>
#include "graphics/Device.h"
#include "backend/Display.h"
#include "backend/Output.h"
#include "render/TestRenderer.h"

struct RenderOutput {
    Output* output;
    vk::Rect2D region;
    TestRenderer renderer;
    std::vector<vk::UniqueFramebuffer> framebuffers;

    RenderOutput(Device& device, Output* output);
    RenderOutput(RenderOutput&&) = default;
    RenderOutput& operator=(RenderOutput&&) = default;

    void render();
};

struct DeviceRenderer {
    Device& device;

    std::vector<RenderOutput> outputs;

    DeviceRenderer(Display* display, size_t gpu, size_t outputs);
    DeviceRenderer(DeviceRenderer&&) = default;
    DeviceRenderer& operator=(DeviceRenderer&&) = default;
    ~DeviceRenderer();

    void render();
    void recreate(size_t output);
};

#endif

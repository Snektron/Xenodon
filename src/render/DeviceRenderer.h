#ifndef _XENODON_RENDER_DEVICERENDERER_H
#define _XENODON_RENDER_DEVICERENDERER_H

#include <memory>
#include <vector>
#include <cstddef>
#include <vulkan/vulkan.h>
#include "graphics/core/Device.h"
#include "backend/Display.h"
#include "backend/Output.h"
#include "render/TestRenderer.h"

struct RenderOutput {
    const RenderDevice& rendev;
    Output* output;
    vk::Rect2D region;
    std::unique_ptr<TestRenderer> renderer;
    std::vector<vk::UniqueCommandBuffer> command_buffers;
    std::vector<vk::UniqueFramebuffer> framebuffers;

    RenderOutput(const RenderDevice& rendev, Output* output);

    void render();
};

struct DeviceRenderer {
    const RenderDevice& rendev;

    std::vector<std::unique_ptr<RenderOutput>> outputs;

    DeviceRenderer(Display* display, size_t device);
    ~DeviceRenderer();

    void render();
    void recreate(size_t output);
};

#endif

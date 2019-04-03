#ifndef _XENODON_BACKEND_HEADLESS_HEADLESSOUTPUT_H
#define _XENODON_BACKEND_HEADLESS_HEADLESSOUTPUT_H

#include <functional>
#include <cstdint>
#include "graphics/core/PhysicalDevice.h"
#include "graphics/Device.h"
#include "graphics/RenderTarget.h"
#include "backend/Output.h"
#include "backend/headless/HeadlessConfig.h"

using Pixel = uint32_t;

class HeadlessOutput final: public Output {
    vk::Rect2D render_region;
    Device device;
    RenderTarget render_target;
    vk::UniqueCommandBuffer command_buffer;

public:
    HeadlessOutput(const PhysicalDevice& gpu, vk::Rect2D render_region);

    uint32_t num_swap_images() const override;
    SwapImage swap_image(uint32_t index) const override;
    vk::Result present(Swapchain::PresentCallback f) override;

    vk::Rect2D region() const override;
    vk::AttachmentDescription color_attachment_descr() const override;

    void download(Pixel* pixels, size_t stride);

    friend class HeadlessDisplay;
};

#endif

#ifndef _XENODON_PRESENT_HEADLESS_HEADLESSSCREEN_H
#define _XENODON_PRESENT_HEADLESS_HEADLESSSCREEN_H

#include <cstdint>
#include "graphics/Device.h"
#include "graphics/RenderTarget.h"
#include "present/Screen.h"
#include "present/headless/HeadlessConfig.h"

using Pixel = uint32_t;

class HeadlessScreen final: public Screen {
    vk::Rect2D render_region;
    Device device;
    RenderTarget render_target;
    vk::UniqueCommandBuffer command_buffer;

public:
    HeadlessScreen(vk::PhysicalDevice gpu, vk::Rect2D render_region);

    uint32_t num_swap_images() const override;
    SwapImage swap_image(uint32_t index) const override;
    vk::Result present(Swapchain::PresentCallback f) override;

    vk::Rect2D region() const override;
    vk::AttachmentDescription color_attachment_descr() const override;

    std::vector<Pixel> download();

    friend class HeadlessDisplay;
};

#endif

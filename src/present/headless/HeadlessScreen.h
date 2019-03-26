#ifndef _XENODON_PRESENT_HEADLESS_HEADLESSSCREEN_H
#define _XENODON_PRESENT_HEADLESS_HEADLESSSCREEN_H

#include "graphics/Device.h"
#include "present/Screen.h"

class HeadlessScreen final: public Screen {
public:
    uint32_t num_swap_images() const override;
    SwapImage swap_image(uint32_t index) const override;
    vk::Result present(Swapchain::PresentCallback f) override;

    vk::Rect2D region() const override;
    vk::AttachmentDescription color_attachment_descr() const override;
};

#endif

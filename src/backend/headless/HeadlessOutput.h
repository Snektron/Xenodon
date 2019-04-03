#ifndef _XENODON_BACKEND_HEADLESS_HEADLESSOUTPUT_H
#define _XENODON_BACKEND_HEADLESS_HEADLESSOUTPUT_H

#include <functional>
#include <cstdint>
#include "graphics/core/PhysicalDevice.h"
#include "graphics/core/Device.h"
#include "graphics/memory/Image.h"
#include "backend/RenderDevice.h"
#include "backend/Output.h"
#include "backend/headless/HeadlessConfig.h"

using Pixel = uint32_t;

class HeadlessOutput final: public Output {
    vk::Rect2D render_region;
    RenderDevice rendev;
    vk::UniqueFence frame_fence;

    Image render_target;
    vk::UniqueImageView render_target_view;

public:
    HeadlessOutput(const PhysicalDevice& physdev, vk::Rect2D render_region);

    uint32_t num_swap_images() const override;
    uint32_t current_swap_index() const override;
    SwapImage swap_image(uint32_t index) override;
    vk::Rect2D region() const override;
    vk::AttachmentDescription color_attachment_descr() const override;

    void synchronize() const;
    void download(Pixel* pixels, size_t stride);

    RenderDevice& render_device() {
        return this->rendev;
    }
};

#endif

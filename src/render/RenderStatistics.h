#ifndef _XENODON_RENDER_RENDERSTATISTICS_H
#define _XENODON_RENDER_RENDERSTATISTICS_H

#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "backend/Display.h"
#include "backend/RenderDevice.h"
#include "utility/Span.h"

class RenderStatistics {
public:
    struct Statistics {
        size_t total_rays;
    };

private:
    const RenderDevice* rendev;
    vk::UniqueQueryPool query_pool;
    size_t total_rays;

public:
    RenderStatistics(Display* display, size_t device_index);
    void pre_dispatch(size_t output_index, vk::CommandBuffer cmd_buf);
    void post_dispatch(size_t output_index, vk::CommandBuffer cmd_buf);
    void collect();
};

#endif

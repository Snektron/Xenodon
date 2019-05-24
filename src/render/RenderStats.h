#ifndef _XENODON_RENDER_RENDERSTATS_H
#define _XENODON_RENDER_RENDERSTATS_H

#include <vector>
#include <limits>
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "backend/Display.h"
#include "backend/RenderDevice.h"

struct RenderStats {
    size_t total_rays = 0;
    size_t outputs = 0;

    // Sum of render times over all outputs, in ms
    float total_render_time = 0;

    // The minimum and maximum time a shader took, in ms
    float max_render_time = 0;
    float min_render_time = std::numeric_limits<float>::max();

    RenderStats& combine(const RenderStats& other);
};

class RenderStatsCollector {
    const RenderDevice* rendev;
    vk::UniqueQueryPool query_pool;
    std::vector<uint64_t> timestamp_buffer;
    RenderStats current_stats;

public:
    RenderStatsCollector(Display* display, size_t device_index);
    void pre_dispatch(size_t output_index, vk::CommandBuffer cmd_buf);
    void post_dispatch(size_t output_index, vk::CommandBuffer cmd_buf);
    void collect();

    const RenderStats& stats() const {
        return this->current_stats;
    }
};

#endif

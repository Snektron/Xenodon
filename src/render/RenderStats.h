#ifndef _XENODON_RENDER_RENDERSTATS_H
#define _XENODON_RENDER_RENDERSTATS_H

#include <vector>
#include <limits>
#include <chrono>
#include <filesystem>
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "backend/Display.h"
#include "backend/RenderDevice.h"

struct RenderStats {
    size_t total_rays = 0;
    size_t outputs = 0;

    // Sum of render times over all outputs, in ms
    double total_render_time = 0;

    // The minimum and maximum time a shader took, in ms
    double max_render_time = 0;
    double min_render_time = std::numeric_limits<double>::max();

    RenderStats& combine(const RenderStats& other);
    double mrays_per_s() const;
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

class RenderStatsAccumulator {
    std::vector<RenderStats> all_stats;
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point stop_time;

public:
    void start();
    void stop();

    void operator()(const RenderStats& stats) {
        this->all_stats.push_back(stats);
    }

    size_t total_rays() const;
    double total_render_time() const;
    double mrays_per_s() const;
    std::chrono::duration<double> total_time() const;
    double fps() const;
    void save(std::filesystem::path path) const;
};

#endif

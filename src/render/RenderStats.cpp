#include "render/RenderStats.h"
#include <array>
#include <algorithm>
#include <fstream>
#include <fmt/format.h>
#include "core/Error.h"

namespace {
    // For each output, 2 queries must be made: start and end time
    constexpr const uint32_t QUERY_COUNT = 2;
}

RenderStats& RenderStats::combine(const RenderStats& other) {
    this->total_rays += other.total_rays;
    this->outputs += other.outputs;
    this->total_render_time += other.total_render_time;
    this->max_render_time = std::max(this->max_render_time, other.max_render_time);
    this->min_render_time = std::min(this->min_render_time, other.min_render_time);
    return *this;
}

double RenderStats::mrays_per_s() const {
    return static_cast<double>(this->total_rays) / (this->total_render_time * 1'000);
}

RenderStatsCollector::RenderStatsCollector(Display* display, size_t device_index):
    rendev(&display->render_device(device_index)) {

    this->current_stats.outputs = this->rendev->outputs;
    for (size_t output_index = 0; output_index < this->rendev->outputs; ++output_index) {
        const auto region = display->output(device_index, output_index)->region();
        this->current_stats.total_rays += region.extent.width * region.extent.height;
    }

    const uint32_t query_count = this->rendev->outputs * QUERY_COUNT;

    this->query_pool = this->rendev->device->createQueryPoolUnique({
        {},
        vk::QueryType::eTimestamp,
        query_count
    });

    this->timestamp_buffer.resize(query_count);
}

void RenderStatsCollector::pre_dispatch(size_t output_index, vk::CommandBuffer cmd_buf) {
    uint32_t query_index = static_cast<uint32_t>(output_index) * QUERY_COUNT;
    cmd_buf.resetQueryPool(this->query_pool.get(), query_index, QUERY_COUNT);
    // Submit begin query
    cmd_buf.writeTimestamp(vk::PipelineStageFlagBits::eComputeShader, this->query_pool.get(), query_index);
}

void RenderStatsCollector::post_dispatch(size_t output_index, vk::CommandBuffer cmd_buf) {
    uint32_t query_index = static_cast<uint32_t>(output_index) * QUERY_COUNT;
    // Submit end query
    cmd_buf.writeTimestamp(vk::PipelineStageFlagBits::eComputeShader, this->query_pool.get(), query_index + 1);
}

void RenderStatsCollector::collect() {
    // This function should be called after Display::swap_buffers, to make sure all command queues are processed

    this->rendev->device->getQueryPoolResults(
        this->query_pool.get(),
        0,
        static_cast<uint32_t>(this->timestamp_buffer.size()),
        static_cast<uint32_t>(this->timestamp_buffer.size()) * sizeof(uint64_t),
        this->timestamp_buffer.data(),
        sizeof(uint64_t),
        vk::QueryResultFlagBits::e64
    );

    this->current_stats.total_render_time = 0;
    this->current_stats.max_render_time = 0;
    this->current_stats.min_render_time = std::numeric_limits<double>::max();

    for (size_t i = 0; i < this->timestamp_buffer.size(); i += QUERY_COUNT) {
        uint64_t diff = this->timestamp_buffer[i + 1] - this->timestamp_buffer[i];
        double time = static_cast<double>(diff) * static_cast<double>(this->rendev->timestamp_period) / 1'000'000.0;
        this->current_stats.total_render_time += time;
        this->current_stats.max_render_time = std::max(this->current_stats.max_render_time, time);
        this->current_stats.min_render_time = std::min(this->current_stats.min_render_time, time);
    }
}

void RenderStatsAccumulator::start() {
    this->start_time = std::chrono::high_resolution_clock::now();
}

void RenderStatsAccumulator::stop() {
    this->stop_time = std::chrono::high_resolution_clock::now();
}

size_t RenderStatsAccumulator::total_rays() const {
    size_t total_rays = 0;
    for (const auto& stats : this->all_stats) {
        total_rays += stats.total_rays;
    }

    return total_rays;
}

double RenderStatsAccumulator::total_render_time() const {
    double total_render_time = 0;
    for (const auto& stats : this->all_stats) {
        total_render_time += stats.total_render_time;
    }

    return total_render_time;
}

double RenderStatsAccumulator::mrays_per_s() const {
    return static_cast<double>(this->total_rays()) / (this->total_render_time() * 1'000);
}

std::chrono::duration<double> RenderStatsAccumulator::total_time() const {
    return this->stop_time - this->start_time;
}

double RenderStatsAccumulator::fps() const {
    return static_cast<double>(this->frames()) / this->total_time().count();
}

size_t RenderStatsAccumulator::frames() const {
    return this->all_stats.size();
}

void RenderStatsAccumulator::save(std::filesystem::path path) const {
    fmt::memory_buffer out;

    fmt::format_to(out, "total rays: {}\n", this->total_rays());
    fmt::format_to(out, "total render time: {}\n", this->total_render_time());
    fmt::format_to(out, "total mray/s: {}\n", this->mrays_per_s());
    fmt::format_to(out, "average fps: {}\n", this->fps());
    fmt::format_to(out, "frames: {}\n", this->frames());
    fmt::format_to(out, "# Frame number: total rays, outputs, total render time, max render time, min render time, mray/s\n");

    for (size_t frame_index = 0; frame_index < this->frames(); ++frame_index) {
        const auto& frame = this->all_stats[frame_index];

        fmt::format_to(
            out,
            "frame {}: {} rays, {}, {} ms, {} ms, {} ms, {} mray/s\n",
            frame_index,
            frame.total_rays,
            frame.outputs,
            frame.total_render_time,
            frame.max_render_time,
            frame.min_render_time,
            frame.mrays_per_s()
        );
    }

    auto out_file = std::ofstream(path);
    if (!out_file) {
        throw Error("Failed to open render stats output path '{}'", path.native());
    }

    out_file << fmt::to_string(out);
}
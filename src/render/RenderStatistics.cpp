#include "render/RenderStatistics.h"
#include <array>
#include <cstdint>
#include <fmt/format.h>

namespace {
    // For each output, 2 queries must be made: start and end time
    constexpr const uint32_t QUERY_COUNT = 2;
}

RenderStatistics::RenderStatistics(Display* display, size_t device_index):
    rendev(&display->render_device(device_index)),
    total_rays(0) {

    uint32_t total_images = 0;
    for (size_t output_index = 0; output_index < this->rendev->outputs; ++output_index) {
        auto* output = display->output(device_index, output_index);
        total_images += output->num_swap_images();
        const auto region = output->region();
        this->total_rays += region.extent.width * region.extent.height;
    }

    const uint32_t query_count = total_images * QUERY_COUNT;

    this->query_pool = this->rendev->device->createQueryPoolUnique({
        {},
        vk::QueryType::eTimestamp,
        query_count
    });
}

void RenderStatistics::pre_dispatch(size_t output_index, vk::CommandBuffer cmd_buf) {
    uint32_t query_index = static_cast<uint32_t>(output_index) * QUERY_COUNT;
    cmd_buf.resetQueryPool(this->query_pool.get(), query_index, QUERY_COUNT);
    // Submit first query
    cmd_buf.writeTimestamp(vk::PipelineStageFlagBits::eComputeShader, this->query_pool.get(), query_index);
}

void RenderStatistics::post_dispatch(size_t output_index, vk::CommandBuffer cmd_buf) {
    uint32_t query_index = static_cast<uint32_t>(output_index) * QUERY_COUNT;
    // Submit second query
    cmd_buf.writeTimestamp(vk::PipelineStageFlagBits::eComputeShader, this->query_pool.get(), query_index + 1);
}

void RenderStatistics::collect() {
    // This function should be called after Display::swap_buffers, to make sure all command queues are processed

    auto timestamps = std::array<uint64_t, 2>();
    this->rendev->device->getQueryPoolResults(
        this->query_pool.get(),
        0,
        2,
        vk::ArrayProxy(timestamps),
        sizeof(uint64_t),
        vk::QueryResultFlagBits::e64
    );

    float diff = static_cast<float>(timestamps[1] - timestamps[0]) * this->rendev->timestamp_period;

    fmt::print(
        "render time: {} ms, rays: {}, Mrays/s: {}\n",
        diff / 1'000'000.f,
        this->total_rays,
        static_cast<float>(this->total_rays) / (diff / 1'000'000'000.f) / 1'000'000.f
    );
}

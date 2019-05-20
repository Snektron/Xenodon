#include "render/DdaRaytraceAlgorithm.h"
#include <utility>
#include "resources.h"
#include "graphics/utility.h"

namespace {
    const auto DDA_BINDINGS = std::array {
        Binding {
            2,
            vk::DescriptorType::eCombinedImageSampler
        }
    };
}

DdaRaytraceResources::DdaRaytraceResources(const RenderDevice& rendev, const Grid& grid):
    grid_texture(
        rendev.device,
        vk::Format::eR8G8B8A8Unorm,
        vk::Extent3D{
            static_cast<uint32_t>(grid.dimensions().x),
            static_cast<uint32_t>(grid.dimensions().y),
            static_cast<uint32_t>(grid.dimensions().z)
        },
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
    ),
    sampler(rendev.device->createSamplerUnique({
        {},
        vk::Filter::eNearest,
        vk::Filter::eNearest,
        vk::SamplerMipmapMode::eNearest,
        vk::SamplerAddressMode::eClampToBorder,
        vk::SamplerAddressMode::eClampToBorder,
        vk::SamplerAddressMode::eClampToBorder
    })) {

    const auto copy_info = vk::BufferImageCopy(
        0,
        0,
        0,
        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
        {0, 0, 0},
        vk::Extent3D{
            static_cast<uint32_t>(grid.dimensions().x),
            static_cast<uint32_t>(grid.dimensions().y),
            static_cast<uint32_t>(grid.dimensions().z)
        }
    );

    const auto pixels = grid.pixels();

    auto staging_buffer = Buffer<Pixel>(
        rendev.device,
        pixels.size(),
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    {
        auto* mapping = staging_buffer.map(0, pixels.size());

        for (size_t i = 0; i < pixels.size(); ++i) {
            mapping[i] = pixels[i];
        }

        staging_buffer.unmap();
    }

    rendev.compute_command_pool.one_time_submit([&, this](vk::CommandBuffer cmd_buf) {
        const auto initial_state = ImageState{
            vk::ImageLayout::eUndefined,
            vk::PipelineStageFlagBits::eTopOfPipe
        };

        const auto upload_state = ImageState{
            vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits::eTransfer,
            vk::AccessFlagBits::eTransferWrite
        };

        const auto render_state = ImageState{
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::AccessFlagBits::eShaderRead
        };

        image_transition(cmd_buf, this->grid_texture.get(), initial_state, upload_state);

        cmd_buf.copyBufferToImage(
            staging_buffer.get(),
            this->grid_texture.get(),
            upload_state.layout,
            copy_info
        );

        image_transition(cmd_buf, this->grid_texture.get(), upload_state, render_state);
    });
}

void DdaRaytraceResources::update_descriptors(vk::DescriptorSet set) const {
    const auto image_info = vk::DescriptorImageInfo(
        this->sampler.get(),
        this->grid_texture.view(),
        vk::ImageLayout::eShaderReadOnlyOptimal
    );

    const auto descriptor_write = vk::WriteDescriptorSet(
        set,
        DDA_BINDINGS[0].binding,
        0,
        1,
        DDA_BINDINGS[0].type,
        &image_info,
        nullptr,
        nullptr
    );

    this->grid_texture.device().updateDescriptorSets(descriptor_write, nullptr);
}

DdaRaytraceAlgorithm::DdaRaytraceAlgorithm(std::shared_ptr<Grid> grid):
    grid(grid) {
}

std::string_view DdaRaytraceAlgorithm::shader() const {
    return resources::open("resources/dda.comp");
}

Span<Binding> DdaRaytraceAlgorithm::bindings() const {
    return DDA_BINDINGS;
}

std::unique_ptr<RenderResources> DdaRaytraceAlgorithm::upload_resources(const RenderDevice& rendev) const {
    return std::make_unique<DdaRaytraceResources>(rendev, *this->grid.get());
}

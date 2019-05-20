#include "render/DdaRaytraceAlgorithm.h"
#include <utility>
#include "resources.h"

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

     auto copy_info = vk::BufferImageCopy(
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

    auto uploader = this->grid_texture.upload(
        grid.pixels(),
        copy_info,
        vk::ImageLayout::eTransferDstOptimal
    );

    rendev.compute_command_pool.one_time_submit([&, this](vk::CommandBuffer cmd_buf) {
        auto barrier0 = vk::ImageMemoryBarrier(
            vk::AccessFlags(),
            vk::AccessFlagBits::eTransferRead,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            this->grid_texture.get(),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        );

        Texture3D::layout_transition(
            cmd_buf,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            barrier0
        );

        uploader(cmd_buf);

        auto barrier1 = vk::ImageMemoryBarrier(
            vk::AccessFlagBits::eTransferWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            this->grid_texture.get(),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        );

        Texture3D::layout_transition(
            cmd_buf,
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eComputeShader,
            barrier1
        );
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

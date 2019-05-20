#include "render/SvoRaytraceAlgorithm.h"

namespace {
    const auto SVO_BINDINGS = std::array {
        Binding {
            2,
            vk::DescriptorType::eStorageBuffer
        }
    };
}

SvoRaytraceResources::SvoRaytraceResources(const RenderDevice& rendev, const Octree& octree):
    node_buffer(
        rendev.device,
        octree.data().size(),
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    ) {

    const Span<Octree::Node> nodes = octree.data();

    const auto copy_info = vk::BufferCopy{
        0,
        0,
        nodes.size() * sizeof(Octree::Node)
    };

    auto staging_buffer = Buffer<Octree::Node>(
        rendev.device,
        nodes.size(),
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    Octree::Node* staging_nodes = staging_buffer.map(0, nodes.size());

    for (size_t i = 0; i < nodes.size(); ++i) {
        staging_nodes[i] = nodes[i];
    }

    staging_buffer.unmap();

    rendev.compute_command_pool.one_time_submit([&](vk::CommandBuffer cmd_buf) {
        cmd_buf.copyBuffer(staging_buffer.get(), node_buffer.get(), copy_info);
    });

    this->size = nodes.size();
}

void SvoRaytraceResources::update_descriptors(vk::DescriptorSet set) const {
    const auto buffer_info = this->node_buffer.descriptor_info(0, this->size);

    const auto descriptor_write = vk::WriteDescriptorSet(
        set,
        SVO_BINDINGS[0].binding,
        0,
        1,
        SVO_BINDINGS[0].type,
        nullptr,
        &buffer_info,
        nullptr
    );

    this->node_buffer.device().updateDescriptorSets(descriptor_write, nullptr);
}

SvoRaytraceAlgorithm::SvoRaytraceAlgorithm(std::string_view shader_source, std::shared_ptr<Octree> octree):
    shader_source(shader_source),
    octree(octree) {
}

std::string_view SvoRaytraceAlgorithm::shader() const {
    return this->shader_source;
}

Span<Binding> SvoRaytraceAlgorithm::bindings() const {
    return SVO_BINDINGS;
}

std::unique_ptr<RenderResources> SvoRaytraceAlgorithm::upload_resources(const RenderDevice& rendev) const {
    return std::make_unique<SvoRaytraceResources>(rendev, *this->octree.get());
}

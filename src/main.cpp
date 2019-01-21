#include <iostream>
#include <array>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <limits>
#include <string_view>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include "utility/ScopeGuard.h"
#include "graphics/PhysicalDevicePicker.h"
#include "resources.h"

namespace {
    constexpr const char* const APP_NAME = "Xenodon";
    constexpr const uint32_t APP_VERSION = VK_MAKE_VERSION(0, 0, 0);

    constexpr const size_t MAX_FRAMES = 2;

    constexpr const std::array DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct PickedDeviceInfo {
        vk::PhysicalDevice physical_device;
        uint32_t graphics_queue_index;
        uint32_t present_queue_index;
    };

    struct SwapchainInfo {
        vk::UniqueSwapchainKHR swapchain;
        vk::Format format;
        vk::Extent2D extent;
    };

    struct Pipeline {
        vk::UniquePipelineLayout layout;
        vk::UniqueRenderPass render_pass;
        vk::UniquePipeline pipeline;
    };
}

vk::UniqueInstance create_instance() {
    auto app_info = vk::ApplicationInfo(
        APP_NAME,
        APP_VERSION,
        nullptr,
        0,
        VK_API_VERSION_1_1
    );

    uint32_t ext_count;
    const auto** extensions = glfwGetRequiredInstanceExtensions(&ext_count);

    auto create_info = vk::InstanceCreateInfo(
        {},
        &app_info,
        0,
        nullptr,
        ext_count,
        extensions
    );

    return vk::createInstanceUnique(create_info);
}

void pick_physical_device2(vk::Instance instance, vk::SurfaceKHR surface) {
    PhysicalDevicePicker(instance)
        .filter([](auto device) {
            return device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
        })
        .with_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        .with_surface_support(surface);
}

PickedDeviceInfo pick_physical_device(vk::UniqueInstance& instance, vk::SurfaceKHR surface) {
    auto check_discrete = [](auto&& device) {
        return device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
    };

    auto check_extensions = [begin = DEVICE_EXTENSIONS.begin(), end = DEVICE_EXTENSIONS.end()](auto&& device) {
        auto exts = device.enumerateDeviceExtensionProperties();

        for (auto&& ext : exts) {
            auto cmp_ext = [&](auto&& name) {
                return std::strcmp(name, ext.extensionName) == 0;
            };

            if (std::find_if(begin, end, cmp_ext) != end)
                return true;
        }

        return false;
    };

    auto check_surface_caps = [&](auto&& device) -> bool {
        auto formats = device.getSurfaceFormatsKHR(surface);
        auto present_modes = device.getSurfacePresentModesKHR(surface);

        return !formats.empty() && !present_modes.empty();
    };

    auto physical_devices = instance->enumeratePhysicalDevices();

    auto graphics_supported = std::vector<uint32_t>();
    auto present_supported = std::vector<uint32_t>();

    for (auto&& device : physical_devices) {
        if (!check_discrete(device)
            || !check_extensions(device)
            || !check_surface_caps(device))
            continue;

        graphics_supported.clear();
        present_supported.clear();
        auto queue_families = device.getQueueFamilyProperties();

        uint32_t num_queues = static_cast<uint32_t>(queue_families.size());
        for (uint32_t i = 0; i < num_queues; ++i) {
            const auto& props = queue_families[i];
            if (props.queueFlags & vk::QueueFlagBits::eGraphics)
                graphics_supported.push_back(i);

            if (device.getSurfaceSupportKHR(i, surface))
                present_supported.push_back(i);
        }

        if (graphics_supported.empty() || present_supported.empty())
            continue;

        auto git = graphics_supported.begin();
        auto pit = present_supported.begin();

        // Check if theres a queue with both supported
        while (git != graphics_supported.end() && pit != present_supported.end()) {
            uint32_t g = *git;
            uint32_t p = *pit;

            if (g == p)
                return {device, g, p};
            else if (g < p)
                ++git;
            else
                ++pit;
        }

        // No queue with both supported, but both are supported in any queue, so just take the first of both
        return {device, graphics_supported[0], present_supported[0]};
    }

    throw std::runtime_error("Failed to find a suitable physical device");
}

vk::UniqueDevice initialize_device(PickedDeviceInfo& picked) {
    float priority = 1.0f;

    auto queue_create_infos = std::array<vk::DeviceQueueCreateInfo, 2>();
    uint32_t queues = 1;

    queue_create_infos[0] = vk::DeviceQueueCreateInfo(
        {},
        picked.graphics_queue_index,
        1,
        &priority
    );

    if (picked.graphics_queue_index != picked.present_queue_index) {
        queues = 2;
        queue_create_infos[1] = vk::DeviceQueueCreateInfo(
            {},
            picked.present_queue_index,
            1,
            &priority
        );
    }
    auto device_create_info = vk::DeviceCreateInfo(
        {},
        queues,
        queue_create_infos.data(),
        0,
        nullptr,
        static_cast<uint32_t>(DEVICE_EXTENSIONS.size()),
        DEVICE_EXTENSIONS.data()
    );

    return picked.physical_device.createDeviceUnique(device_create_info);
}

vk::UniqueSurfaceKHR create_surface(vk::UniqueInstance& instance, GLFWwindow* window) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(VkInstance(instance.get()), window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface");

    vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderStatic> deleter(instance.get());
    return vk::UniqueSurfaceKHR(vk::SurfaceKHR(surface), deleter);
}

vk::SurfaceFormatKHR pick_surface_format(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
    auto formats = physical_device.getSurfaceFormatsKHR(surface);
    auto preferred_format = vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};

    // Can we pick any format?
    if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined)
        return preferred_format;

    // Check if the preferred format is available
    for (auto&& format : formats) {
        if (format == preferred_format)
            return format;
    }

    // Pick any format
    return formats[0];
}

vk::PresentModeKHR pick_present_mode(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
    auto present_modes = physical_device.getSurfacePresentModesKHR(surface);

    // check for triple buffering support
    if (std::find(present_modes.begin(), present_modes.end(), vk::PresentModeKHR::eMailbox) != present_modes.end())
        return vk::PresentModeKHR::eMailbox;

    // Immediate mode
    if (std::find(present_modes.begin(), present_modes.end(), vk::PresentModeKHR::eImmediate) != present_modes.end())
        return vk::PresentModeKHR::eImmediate;

    // Double buffering, guaranteed to be available but not always supported
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D pick_swap_extent(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, GLFWwindow* window) {
    auto caps = physical_device.getSurfaceCapabilitiesKHR(surface);

    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return caps.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        return {
            std::clamp(caps.minImageExtent.width, caps.maxImageExtent.width, static_cast<uint32_t>(width)),
            std::clamp(caps.minImageExtent.height, caps.maxImageExtent.height, static_cast<uint32_t>(height)),
        };
    }
}

SwapchainInfo create_swap_chain(PickedDeviceInfo& picked, vk::Device device, vk::SurfaceKHR surface, GLFWwindow* window) {
    auto surface_format = pick_surface_format(picked.physical_device, surface);
    auto present_mode = pick_present_mode(picked.physical_device, surface);
    auto extent = pick_swap_extent(picked.physical_device, surface, window);

    auto caps = picked.physical_device.getSurfaceCapabilitiesKHR(surface);

    uint32_t image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0)
        image_count = std::min(caps.maxImageCount, image_count);

    auto create_info = vk::SwapchainCreateInfoKHR(
        {},
        surface,
        image_count,
        surface_format.format,
        surface_format.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        caps.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        present_mode,
        true,
        nullptr
    );

    auto queue_indices = std::array{picked.graphics_queue_index, picked.present_queue_index};

    if (picked.graphics_queue_index != picked.present_queue_index) {
        create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_indices.size());
        create_info.pQueueFamilyIndices = queue_indices.data();
    }

    return {device.createSwapchainKHRUnique(create_info), surface_format.format, extent};
}

std::vector<vk::UniqueImageView> initialize_views(vk::Device& device, vk::SwapchainKHR& swapchain, vk::Format format) {
    auto swapchain_images = device.getSwapchainImagesKHR(swapchain);
    auto image_views = std::vector<vk::UniqueImageView>();

    auto component_mapping = vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
    auto sub_resource_range = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
    for (auto&& image : swapchain_images) {
        auto create_info = vk::ImageViewCreateInfo(
            {},
            image,
            vk::ImageViewType::e2D,
            format,
            component_mapping,
            sub_resource_range
        );

        image_views.push_back(device.createImageViewUnique(create_info));
    }

    return image_views;
}

vk::UniqueShaderModule create_shader(vk::Device device, const std::string_view& code) {
    return device.createShaderModuleUnique(vk::ShaderModuleCreateInfo(
        {},
        code.size(),
        reinterpret_cast<const uint32_t*>(code.data())
    ));
}

vk::PipelineShaderStageCreateInfo create_shader_info(const vk::ShaderModule& shader, vk::ShaderStageFlagBits stage) {
    return vk::PipelineShaderStageCreateInfo(
        {},
        stage,
        shader,
        "main"
    );
}

Pipeline create_pipeline(vk::Device device, const vk::Extent2D& extent, vk::Format format) {
    auto vertex_shader = create_shader(device, resources::open("resources/test.vert"));
    auto fragment_shader = create_shader(device, resources::open("resources/test.frag"));

    auto shader_stages_infos = std::array{
        create_shader_info(vertex_shader.get(), vk::ShaderStageFlagBits::eVertex),
        create_shader_info(fragment_shader.get(), vk::ShaderStageFlagBits::eFragment)
    };

    auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo();
    auto assembly_info = vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleStrip);

    auto viewport = vk::Viewport(0, 0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0, 1);
    auto scissor = vk::Rect2D({0, 0}, extent);
    auto viewport_info = vk::PipelineViewportStateCreateInfo({}, 1, &viewport, 1, &scissor);

    auto rasterizer_info = vk::PipelineRasterizationStateCreateInfo();
    rasterizer_info.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer_info.lineWidth = 1.0f;

    auto multisample_info = vk::PipelineMultisampleStateCreateInfo();

    auto color_blend_attachment_info = vk::PipelineColorBlendAttachmentState();
    color_blend_attachment_info.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    auto color_blend_info = vk::PipelineColorBlendStateCreateInfo();
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend_attachment_info;

    auto pipeline_layout_info = vk::PipelineLayoutCreateInfo();
    auto pipeline_layout = device.createPipelineLayoutUnique(pipeline_layout_info);

    // Create render pass
    auto color_attachment = vk::AttachmentDescription({}, format);
    color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    auto attachment_ref = vk::AttachmentReference(
        0, // The layout(location = x) of the fragment shader
        vk::ImageLayout::eColorAttachmentOptimal
    );

    auto subpass = vk::SubpassDescription(
        {},
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        1,
        &attachment_ref
    );

    auto dependency = vk::SubpassDependency(
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlags(),
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentRead
    );

    auto render_pass_info = vk::RenderPassCreateInfo(
        {},
        1,
        &color_attachment,
        1,
        &subpass,
        1,
        &dependency
    );

    auto render_pass = device.createRenderPassUnique(render_pass_info);

    auto pipeline_info = vk::GraphicsPipelineCreateInfo(
        {},
        shader_stages_infos.size(),
        shader_stages_infos.data(),
        &vertex_input_info,
        &assembly_info,
        nullptr,
        &viewport_info,
        &rasterizer_info,
        &multisample_info,
        nullptr,
        &color_blend_info,
        nullptr,
        pipeline_layout.get(),
        render_pass.get()
    );

    return {
        std::move(pipeline_layout),
        std::move(render_pass),
        device.createGraphicsPipelineUnique(vk::PipelineCache(), pipeline_info)
    };
}

std::vector<vk::UniqueFramebuffer> create_frame_buffers(vk::Device device, const std::vector<vk::UniqueImageView>& views, const Pipeline& pipeline, const vk::Extent2D& extent) {
    auto frame_buffers = std::vector<vk::UniqueFramebuffer>(views.size());

    for (size_t i = 0; i < views.size(); ++i) {
        auto create_info = vk::FramebufferCreateInfo(
            {},
            pipeline.render_pass.get(),
            1,
            &views[i].get(),
            extent.width,
            extent.height,
            1
        );

        frame_buffers[i] = device.createFramebufferUnique(create_info);
    }

    return frame_buffers;
}

vk::UniqueCommandPool create_command_pool(vk::Device device, uint32_t graphics_queue) {
    auto command_pool_info = vk::CommandPoolCreateInfo({}, graphics_queue);
    return device.createCommandPoolUnique(command_pool_info);
}

std::vector<vk::UniqueCommandBuffer> create_command_buffers(vk::Device device, const std::vector<vk::UniqueFramebuffer>& frame_buffers, vk::CommandPool pool) {
    auto command_buffers_info = vk::CommandBufferAllocateInfo(pool);
    command_buffers_info.commandBufferCount = static_cast<uint32_t>(frame_buffers.size());

    return device.allocateCommandBuffersUnique(command_buffers_info);
}

vk::UniqueSemaphore create_semaphore(vk::Device device) {
    return device.createSemaphoreUnique(vk::SemaphoreCreateInfo());
}

vk::UniqueFence create_fence(vk::Device device) {
    return device.createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
}

struct RenderState {
    vk::UniqueSwapchainKHR swapchain;
    vk::Format format;
    vk::Extent2D extent;
    std::vector<vk::UniqueImageView> image_views;
    Pipeline pipeline;
    std::vector<vk::UniqueFramebuffer> frame_buffers;
    std::vector<vk::UniqueCommandBuffer> command_buffers;
};

RenderState create_render_state(PickedDeviceInfo& physical_device, vk::Device device, vk::SurfaceKHR surface, GLFWwindow* window, vk::CommandPool command_pool) {
    auto [swapchain, format, extent] = create_swap_chain(physical_device, device, surface, window);
    auto image_views = initialize_views(device, swapchain.get(), format);
    auto pipeline = create_pipeline(device, extent, format); 
    auto frame_buffers = create_frame_buffers(device, image_views, pipeline, extent);
    auto command_buffers = create_command_buffers(device, frame_buffers, command_pool);

    return {
        std::move(swapchain),
        format,
        extent,
        std::move(image_views),
        std::move(pipeline),
        std::move(frame_buffers),
        std::move(command_buffers)
    };
}

void render_triangle(RenderState& state, vk::CommandBuffer buf, vk::Framebuffer frame_buffer) {
    const auto begin_info = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
    const auto clear_color = vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{0.f, 0.0f, 0.f, 1.f}));

    buf.begin(&begin_info);

    auto render_pass_begin_info = vk::RenderPassBeginInfo(
        state.pipeline.render_pass.get(),
        frame_buffer,
        vk::Rect2D({0, 0}, state.extent),
        1,
        &clear_color
    );

    buf.beginRenderPass(&render_pass_begin_info, vk::SubpassContents::eInline);
    buf.bindPipeline(vk::PipelineBindPoint::eGraphics, state.pipeline.pipeline.get());
    buf.draw(4, 1, 0, 0);
    buf.endRenderPass();
    buf.end();
}

void render_triangle(RenderState& state) {
    for (size_t i = 0; i < state.frame_buffers.size(); ++i) {
        render_triangle(state, state.command_buffers[i].get(), state.frame_buffers[i].get());
    }
}

int main() {
    using namespace std::literals::chrono_literals;

    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    auto _finalize_glfw = ScopeGuard([] {
        glfwTerminate();
    });

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    auto* window = glfwCreateWindow(
        800,
        600,
        "Vulkan test",
        nullptr,
        nullptr
    );

    auto _finalize_window = ScopeGuard([window] {
        glfwDestroyWindow(window);
    });

    auto instance = create_instance();
    auto surface = create_surface(instance, window);
    auto picked = pick_physical_device(instance, surface.get());
    std::cout << "Picked device '" << picked.physical_device.getProperties().deviceName << '\'' << std::endl;
    auto device = initialize_device(picked);
    auto command_pool = create_command_pool(device.get(), picked.graphics_queue_index);

    auto graphics_queue = device->getQueue(picked.graphics_queue_index, 0);
    auto present_queue = device->getQueue(picked.present_queue_index, 0);

    auto render_state = create_render_state(picked, device.get(), surface.get(), window, command_pool.get());
    render_triangle(render_state);

    auto image_available_sems = std::vector<vk::UniqueSemaphore>(MAX_FRAMES);
    auto render_finished_sems = std::vector<vk::UniqueSemaphore>(MAX_FRAMES);
    auto fences = std::vector<vk::UniqueFence>(MAX_FRAMES);

    for (size_t i = 0; i < MAX_FRAMES; ++i) {
        image_available_sems[i] = create_semaphore(device.get());
        render_finished_sems[i] = create_semaphore(device.get());
        fences[i] = create_fence(device.get());
    }

    auto start = std::chrono::high_resolution_clock::now();
    size_t start_frame = 0;
    size_t total_frames = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        size_t current_frame = total_frames % MAX_FRAMES;
        auto& fence = fences[current_frame].get();
        device->waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
        device->resetFences(fence);

        auto& image_available = image_available_sems[current_frame].get();
        auto& render_finished = render_finished_sems[current_frame].get();

        uint32_t image_index;
        auto result = device->acquireNextImageKHR(render_state.swapchain.get(), std::numeric_limits<uint64_t>::max(), image_available, vk::Fence(), &image_index);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            device->waitIdle();
            render_state = create_render_state(picked, device.get(), surface.get(), window, command_pool.get());
            render_triangle(render_state);
            image_index = device->acquireNextImageKHR(render_state.swapchain.get(), std::numeric_limits<uint64_t>::max(), image_available, vk::Fence()).value;
        } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error("Failed to acquire next image");
        }

        auto wait_stages = std::array{vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput)};
        auto submit_info = vk::SubmitInfo(
            1,
            &image_available,
            wait_stages.data(),
            1,
            &render_state.command_buffers[image_index].get(),
            1,
            &render_finished
        );

        graphics_queue.submit(1, &submit_info, fence);

        auto present_info = vk::PresentInfoKHR(
            1,
            &render_finished,
            1,
            &render_state.swapchain.get(),
            &image_index
        );

        present_queue.presentKHR(&present_info);
        present_queue.waitIdle();

        total_frames++;

        auto now = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration<double>(now - start);

        if (diff > 1s) {
            size_t frames = total_frames - start_frame;
            std::cout << "FPS: " << static_cast<double>(frames) / diff.count() << std::endl;
            start_frame = total_frames;
            start = now;
        }
    }

    device->waitIdle();
}

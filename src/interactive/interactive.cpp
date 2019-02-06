#include "interactive/interactive.h"
#include <iostream>
#include <array>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <limits>
#include <string_view>
#include <chrono>
#include <vector>
#include <memory>
#include <cstring>
#include <cstdlib>
#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include "render/DeviceContext.h"
#include "render/PhysicalDeviceInfo.h"
#include "render/Renderer.h"
#include "interactive/Window.h"
#include "interactive/EventLoop.h"
#include "interactive/Swapchain.h"
#include "utility/ScopeGuard.h"
#include "resources.h"

namespace {
    constexpr const char* const APP_NAME = "Xenodon";
    constexpr const uint32_t APP_VERSION = VK_MAKE_VERSION(0, 0, 0);

    constexpr const size_t MAX_FRAMES = 2;

    constexpr const std::array INSTANCE_EXTENSIONS = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME
    };

    constexpr const std::array DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct PickedDeviceInfo {
        vk::PhysicalDevice physical_device;
        uint32_t graphics_queue_index;
        uint32_t present_queue_index;
    };

    vk::UniqueInstance create_instance() {
        auto app_info = vk::ApplicationInfo(
            APP_NAME,
            APP_VERSION,
            nullptr,
            0,
            VK_API_VERSION_1_1
        );

        auto create_info = vk::InstanceCreateInfo(
            {},
            &app_info,
            0,
            nullptr,
            INSTANCE_EXTENSIONS.size(),
            INSTANCE_EXTENSIONS.data()
        );

        return vk::createInstanceUnique(create_info);
    }

    PickedDeviceInfo pick_physical_device(vk::Instance instance, vk::SurfaceKHR surface) {
        auto physical_devices = instance.enumeratePhysicalDevices();

        auto graphics_supported = std::vector<uint32_t>();
        auto present_supported = std::vector<uint32_t>();

        for (auto&& device : physical_devices) {
            auto info = PhysicalDeviceInfo(device);

            if (!info.supports_extensions(DEVICE_EXTENSIONS)
                || !info.supports_surface(surface)
                || (device.getProperties().deviceType != vk::PhysicalDeviceType::eDiscreteGpu
                    && device.getProperties().deviceType != vk::PhysicalDeviceType::eIntegratedGpu))
                continue;

            graphics_supported.clear();
            present_supported.clear();
            auto queue_families = device.getQueueFamilyProperties();

            uint32_t num_queues = static_cast<uint32_t>(queue_families.size());
            for (uint32_t i = 0; i < num_queues; ++i) {
                if (queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics)
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

    vk::UniqueCommandPool create_command_pool(vk::Device device, uint32_t graphics_queue) {
        auto command_pool_info = vk::CommandPoolCreateInfo({}, graphics_queue);
        return device.createCommandPoolUnique(command_pool_info);
    }

    vk::UniqueSemaphore create_semaphore(vk::Device device) {
        return device.createSemaphoreUnique(vk::SemaphoreCreateInfo());
    }

    vk::UniqueFence create_fence(vk::Device device) {
        return device.createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }

    void render(Renderer& renderer, Swapchain& swapchain) {
        for (auto& frame : swapchain.frames) {
            renderer.render(frame.command_buffer.get(), frame.framebuffer.get());
        }
    }
}

template <typename To, typename From>
MallocPtr<To> event_cast(MallocPtr<From>& from) {
    return MallocPtr<To>(reinterpret_cast<To*>(from.release()));
}

void interactive_main() {
    using namespace std::literals::chrono_literals;

    xcb_connection_t* connection = xcb_connect(nullptr, nullptr);
    auto _destroy_xcb_connection = ScopeGuard([connection]{
        xcb_disconnect(connection);
    });

    auto handler = EventLoop(connection);
    xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    auto window = Window(connection, screen, Window::Mode::WINDOWED, 800, 600);

    auto instance = create_instance();
    auto surface = instance->createXcbSurfaceKHRUnique(window.surface_create_info());

    auto picked = pick_physical_device(instance.get(), surface.get());
    std::cout << "Picked device '" << picked.physical_device.getProperties().deviceName << '\'' << std::endl;
    auto device = initialize_device(picked);

    auto device_context = DeviceContext {
        .physical_device = picked.physical_device,
        .device = device.get(),
        .graphics = Queue(device.get(), picked.graphics_queue_index),
        .present = Queue(device.get(), picked.present_queue_index),
        .graphics_command_pool = create_command_pool(device.get(), picked.graphics_queue_index)
    };

    vk::Extent2D window_extent = window.geometry().extent;

    auto color_attachment = vk::AttachmentDescription({}, vk::Format::eB8G8R8A8Unorm);
    color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    auto renderer = std::make_unique<Renderer>(device_context, vk::Rect2D({0, 0}, window_extent), color_attachment);
    auto swapchain = Swapchain(device_context, surface.get(), window_extent, renderer->final_render_pass());
    render(*renderer, swapchain);

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

    bool quit = false;

    auto poll_events = [&] {
        while (auto event = handler.poll_event()) {
            switch (static_cast<int>(event->response_type) & ~0x80) {
                case XCB_KEY_PRESS: {
                    std::cout << "key press" << std::endl;
                    auto key_press = event_cast<xcb_key_press_event_t>(event);
                    quit = true;
                    break;
                }
                case XCB_CONFIGURE_NOTIFY: {
                    auto config = event_cast<xcb_configure_notify_event_t>(event);
                    window_extent = vk::Extent2D{
                        static_cast<uint32_t>(config->width),
                        static_cast<uint32_t>(config->height)
                    };

                    device->waitIdle();
                    renderer = std::make_unique<Renderer>(device_context, vk::Rect2D({0, 0}, window_extent), color_attachment);
                    swapchain.recreate(window_extent, renderer->final_render_pass());
                    render(*renderer, swapchain);

                    break;
                }
            }
        }
    };

    while (!quit) {
        poll_events();

        size_t current_frame = total_frames % MAX_FRAMES;
        auto& fence = fences[current_frame].get();
        device->waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
        device->resetFences(fence);

        auto& image_available = image_available_sems[current_frame].get();
        auto& render_finished = render_finished_sems[current_frame].get();

        vk::Result result = swapchain.acquire_next_image(image_available);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
            device->waitIdle();
            renderer = std::make_unique<Renderer>(device_context, vk::Rect2D({0, 0}, window_extent), color_attachment);
            swapchain.recreate(window_extent, renderer->final_render_pass());
            result = swapchain.acquire_next_image(image_available);

            if (result == vk::Result::eSuccess) {
                render(*renderer, swapchain);
            }
        }

        if (result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to acquire next image");
        }

        auto wait_stages = std::array{vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput)};
        auto submit_info = vk::SubmitInfo(
            1,
            &image_available,
            wait_stages.data(),
            1,
            &swapchain.active_frame().command_buffer.get(),
            1,
            &render_finished
        );

        device_context.graphics.queue.submit(1, &submit_info, fence);

        swapchain.present(render_finished);

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
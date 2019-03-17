#include "present/xorg/xorg_main.h"
#include <array>
#include <vulkan/vulkan.hpp>
#include "main_loop.h"
#include "version.h"
#include "present/xorg/XorgDisplay.h"

namespace {
    constexpr const std::array REQUIRED_INSTANCE_EXTENSIONS = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
    };
}

void xorg_main(int argc, char* argv[]) {
    auto instance = vk::createInstanceUnique(
        vk::InstanceCreateInfo(
            {},
            &version::APP_INFO,
            0,
            nullptr,
            REQUIRED_INSTANCE_EXTENSIONS.size(),
            REQUIRED_INSTANCE_EXTENSIONS.data()
        )
    );

    auto dispatcher = EventDispatcher();
    auto display = XorgDisplay(instance.get(), dispatcher, 800, 600);

    main_loop(dispatcher, &display);
}

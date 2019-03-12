#include "present/xorg/xorg_main.h"
#include <vulkan/vulkan.hpp>
#include "main_loop.h"
#include "version.h"
#include "present/xorg/XorgDisplay.h"

void xorg_main(int argc, char* argv[]) {
    auto instance = vk::createInstanceUnique(
        vk::InstanceCreateInfo(
            {},
            &version::APP_INFO,
            0,
            nullptr,
            XorgDisplay::REQUIRED_INSTANCE_EXTENSIONS.size(),
            XorgDisplay::REQUIRED_INSTANCE_EXTENSIONS.data()
        )
    );

    auto dispatcher = EventDispatcher();
    auto display = XorgDisplay(instance.get(), dispatcher, 800, 600);

    main_loop(dispatcher, &display);
}

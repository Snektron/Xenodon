#ifndef XENODON_VERSION_H
#define XENODON_VERSION_H

#include <vulkan/vulkan.hpp>

namespace version {
    constexpr const char* NAME = @NAME@;
    constexpr const unsigned MAJOR = @MAJOR@;
    constexpr const unsigned MINOR = @MINOR@;
    constexpr const unsigned PATCH = @PATCH@;

    const auto APP_INFO = vk::ApplicationInfo(
        @NAME@,
        VK_MAKE_VERSION(@MAJOR@, @MINOR@, @PATCH@),
        nullptr,
        0,
        VK_API_VERSION_1_1
    );
}

#endif
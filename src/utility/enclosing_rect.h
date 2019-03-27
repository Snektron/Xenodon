#ifndef _XENODON_UTILITY_ENCLOSING_RECT_H
#define _XENODON_UTILITY_ENCLOSING_RECT_H

#include <algorithm>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "core/Logger.h"

template <typename It, typename F>
vk::Rect2D enclosing_rect(It first, It last, F f) {
    if (first == last)
        return {{0, 0}, {0, 0}};

    vk::Rect2D rect = f(*first);
    ++first;

    ssize_t begin_x = static_cast<ssize_t>(rect.offset.x);
    ssize_t begin_y = static_cast<ssize_t>(rect.offset.y);

    ssize_t end_x = begin_x + static_cast<ssize_t>(rect.extent.width);
    ssize_t end_y = begin_y + static_cast<ssize_t>(rect.extent.height);

    while (first != last) {
        rect = f(*first);
        ++first;

        auto current_begin_x = static_cast<ssize_t>(rect.offset.x);
        auto current_begin_y = static_cast<ssize_t>(rect.offset.y);

        begin_x = std::min(begin_x, current_begin_x);
        begin_y = std::min(begin_y, current_begin_y);

        end_x = std::max(end_x, current_begin_x + static_cast<ssize_t>(rect.extent.width));
        end_y = std::max(end_y, current_begin_y + static_cast<ssize_t>(rect.extent.height));
    }

    return {
        {static_cast<int32_t>(begin_x), static_cast<int32_t>(begin_x)},
        {static_cast<uint32_t>(end_x - begin_x), static_cast<uint32_t>(end_y - begin_y)}
    };
}

#endif

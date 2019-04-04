#ifndef _XENODON_UTILITY_ENCLOSING_RECT_H
#define _XENODON_UTILITY_ENCLOSING_RECT_H

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "core/Logger.h"

inline vk::Rect2D enclosing_rect(vk::Rect2D left, vk::Rect2D right) {
    int32_t x = std::min(left.offset.x, right.offset.x);
    int32_t y = std::min(left.offset.y, right.offset.y);

    uint32_t w = std::max(
        static_cast<uint32_t>(left.offset.x) + left.extent.width,
        static_cast<uint32_t>(right.offset.x) + right.extent.width
    ) - static_cast<uint32_t>(x);

    uint32_t h = std::max(
        static_cast<uint32_t>(left.offset.y) + left.extent.height,
        static_cast<uint32_t>(right.offset.y) + right.extent.height
    ) - static_cast<uint32_t>(y);

    return {{x, y}, {w, h}};
}

template <typename It, typename F, typename = std::enable_if_t<!std::is_same_v<typename std::iterator_traits<It>::value_type, void>>>
vk::Rect2D enclosing_rect(It first, It last, F f) {
    if (first == last)
        return {{0, 0}, {0, 0}};

    vk::Rect2D rect = f(*first);
    ++first;

    ssize_t begin_x = static_cast<ssize_t>(rect.offset.x);
    ssize_t begin_y = static_cast<ssize_t>(rect.offset.y);

    ssize_t end_x = begin_x + rect.extent.width;
    ssize_t end_y = begin_y + rect.extent.height;

    while (first != last) {
        rect = f(*first);
        ++first;

        auto current_begin_x = static_cast<ssize_t>(rect.offset.x);
        auto current_begin_y = static_cast<ssize_t>(rect.offset.y);

        begin_x = std::min(begin_x, current_begin_x);
        begin_y = std::min(begin_y, current_begin_y);

        end_x = std::max(end_x, current_begin_x + rect.extent.width);
        end_y = std::max(end_y, current_begin_y + rect.extent.height);
    }

    return {
        {static_cast<int32_t>(begin_x), static_cast<int32_t>(begin_x)},
        {static_cast<uint32_t>(end_x - begin_x), static_cast<uint32_t>(end_y - begin_y)}
    };
}

#endif

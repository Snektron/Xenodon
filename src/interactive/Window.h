#ifndef _XENODON_INTERACTIVE_WINDOW_H
#define _XENODON_INTERACTIVE_WINDOW_H

#include <xcb/xcb.h>
#include <vulkan/vulkan.hpp>
#include "utility/MallocPtr.h"

class EventLoop;

class Window {
    xcb_connection_t* connection;
    xcb_screen_t* screen;
    xcb_window_t xid;

    Window(xcb_connection_t* connection, xcb_screen_t* screen);

public:
    struct Mode {
        constexpr const static struct Fullscreen {} FULLSCREEN = {};
        constexpr const static struct Windowed {} WINDOWED = {};
    };

    Window(xcb_connection_t* connection, xcb_screen_t* screen, Mode::Fullscreen);
    Window(xcb_connection_t* connection, xcb_screen_t* screen, Mode::Windowed, uint16_t width, uint16_t height);

    Window(const Window&) = delete;
    Window(Window&&) = default;

    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&) = default;

    ~Window();
    vk::XcbSurfaceCreateInfoKHR surface_create_info() const;
    vk::Rect2D geometry() const;

    friend class EventHandler;
    friend class Display;
};

#endif

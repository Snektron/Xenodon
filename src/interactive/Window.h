#ifndef _XENODON_INTERACTIVE_WINDOW_H
#define _XENODON_INTERACTIVE_WINDOW_H

#include <string_view>
#include <xcb/xcb.h>
#include <vulkan/vulkan.hpp>
#include "utility/MallocPtr.h"

using AtomReply = MallocPtr<xcb_intern_atom_reply_t>;

struct WindowContext {
    xcb_connection_t* connection;
    AtomReply atom_wm_delete_window;

    WindowContext(xcb_connection_t* connection);
    AtomReply atom(bool only_if_exists, const std::string_view& str) const;
};

class Window {
    xcb_connection_t* connection;
    xcb_screen_t* screen;
    xcb_window_t xid;

    Window(WindowContext& window_context, xcb_screen_t* screen);

public:
    struct Mode {
        constexpr const static struct Fullscreen {} FULLSCREEN = {};
        constexpr const static struct Windowed {} WINDOWED = {};
    };

    Window(WindowContext& window_context, xcb_screen_t* screen, Mode::Fullscreen);
    Window(WindowContext& window_context, xcb_screen_t* screen, Mode::Windowed, uint16_t width, uint16_t height);

    Window(const Window&) = delete;
    Window(Window&& other);

    Window& operator=(const Window&) = delete;
    Window& operator=(Window&& other);

    ~Window();

    vk::XcbSurfaceCreateInfoKHR surface_create_info() const;
    vk::Rect2D geometry() const;

    friend class Display;
};

#endif

#ifndef _XENODON_INTERACTIVE_WINDOW_H
#define _XENODON_INTERACTIVE_WINDOW_H

#include <xcb/xcb.h>
#include <vulkan/vulkan.hpp>
#include "utility/MallocPtr.h"

class Window {
private:
    using AtomReply = MallocPtr<xcb_intern_atom_reply_t>;

    xcb_connection_t* connection;
    xcb_screen_t* screen;
    xcb_window_t window;
    AtomReply atom_wm_delete_window;

public:
    const uint16_t width, height;

    Window(xcb_connection_t* connection, xcb_screen_t* screen);
    ~Window();
    vk::XcbSurfaceCreateInfoKHR surface_create_info() const;

private:
    AtomReply atom(bool only_if_exists, const std::string_view& str) const;
};

#endif

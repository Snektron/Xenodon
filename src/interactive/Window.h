#ifndef _XENODON_INTERACTIVE_WINDOW_H
#define _XENODON_INTERACTIVE_WINDOW_H

#include <xcb/xcb.h>
#include "utility/MallocPtr.h"

class Window {
public:
    using AtomReply = MallocPtr<xcb_intern_atom_reply_t>;

private:
    xcb_connection_t* connection;
    xcb_screen_t* screen;
    xcb_window_t window;
    AtomReply atom_wm_delete_window;

public:
    const uint16_t width, height;

    Window(xcb_connection_t* connection, xcb_screen_t* screen);
    ~Window();

    void set_title(const std::string_view& title) const;
};

#endif

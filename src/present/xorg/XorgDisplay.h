#ifndef _XENODON_PRESENT_XORG_XORGDISPLAY_H
#define _XENODON_PRESENT_XORG_XORGDISPLAY_H

#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include "present/Display.h"
#include "present/Event.h"
#include "utility/MallocPtr.h"

class XorgDisplay: public Display {
    using AtomReply = MallocPtr<xcb_intern_atom_reply_t>;

    vk::Instance instance;
    EventDispatcher* dispatcher;
    xcb_connection_t* connection;
    int preferred_screen_index;
    AtomReply atom_wm_delete_window;
    xcb_window_t window;

public:
    XorgDisplay(vk::Instance instance, EventDispatcher& dispatcher, uint16_t width, uint16_t height);
    XorgDisplay(XorgDisplay&& other);
    XorgDisplay& operator=(XorgDisplay&& other);
    ~XorgDisplay() override;

    XorgDisplay(const XorgDisplay&) = delete;
    XorgDisplay& operator=(const XorgDisplay&) = delete;

    void poll_events() override;

private:
    void handle_event(const xcb_generic_event_t& event);
    AtomReply atom(bool only_if_exists, const std::string_view& str) const;
};

#endif

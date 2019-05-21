#ifndef _XENODON_BACKEND_XORG_WINDOW_H
#define _XENODON_BACKEND_XORG_WINDOW_H

#include <functional>
#include <memory>
#include <string_view>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include "backend/Event.h"
#include "utility/MallocPtr.h"

class Window {
    using AtomReply = MallocPtr<xcb_intern_atom_reply_t>;

    struct XcbDisconnect {
        void operator()(xcb_connection_t* connection) {
            xcb_disconnect(connection);
        }
    };

    struct FreeXcbKeySymbols {
        void operator()(xcb_key_symbols_t* symbols) {
            xcb_key_symbols_free(symbols);
        }
    };

    using XcbConnectionPtr = std::unique_ptr<xcb_connection_t, XcbDisconnect>;
    using XcbKeySymbolsPtr = std::unique_ptr<xcb_key_symbols_t, FreeXcbKeySymbols>;

    XcbConnectionPtr connection;
    xcb_window_t window;
    AtomReply atom_wm_delete_window;
    EventDispatcher* dispatcher;
    uint16_t width, height;
    XcbKeySymbolsPtr key_symbols;
    int16_t mouse_x, mouse_y;

public:
    using ResizeCallback = std::function<void(vk::Extent2D resize_callback)>;

    Window(EventDispatcher& dispatcher, vk::Extent2D extent);
    Window(EventDispatcher& dispatcher, const char* displayname, bool override_redirect);

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other) = default;
    Window& operator=(Window&& other) = default;

    ~Window();

    void poll_events(ResizeCallback cbk);
    void handle_event(ResizeCallback cbk, const xcb_generic_event_t& event);

    std::pair<xcb_connection_t*, xcb_window_t> x_handles();
    vk::Extent2D extent() const;

private:
    xcb_screen_t* init_connection(const char* displayname);
    void init_window(xcb_screen_t* screen, vk::Extent2D extent, bool override_redirect);
    AtomReply atom(bool only_if_exists, const std::string_view& str) const;
};

#endif

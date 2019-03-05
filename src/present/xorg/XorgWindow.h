#ifndef _XENODON_PRESENT_XORG_XORGWINDOW_H
#define _XENODON_PRESENT_XORG_XORGWINDOW_H

#include <memory>
#include <string_view>
#include <cstdint>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include "present/Event.h"
#include "utility/MallocPtr.h"

class XorgScreen;

class XorgWindow {
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

public:
    XorgWindow(EventDispatcher& dispatcher, uint16_t width, uint16_t height);

    XorgWindow(const XorgWindow&) = delete;
    XorgWindow& operator=(const XorgWindow&) = delete;

    XorgWindow(XorgWindow&& other) = default;
    XorgWindow& operator=(XorgWindow&& other) = default;

    ~XorgWindow();

    void poll_events(XorgScreen& screen);
    void handle_event(XorgScreen& screen, const xcb_generic_event_t& event);

    std::pair<xcb_connection_t*, xcb_window_t> x_handles();
private:
    AtomReply atom(bool only_if_exists, const std::string_view& str) const;
};

#endif

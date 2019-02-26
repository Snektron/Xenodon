#ifndef _XENODON_INTERACTIVE_EVENTLOOP_H
#define _XENODON_INTERACTIVE_EVENTLOOP_H

#include <unordered_map>
#include <memory>
#include <functional>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include "interactive/Window.h"

enum class Action {
    Press,
    Release
};

class EventLoop {
    using ActionCallback = std::function<void(Action)>;
    using AxisCallback = std::function<void(double)>;
    using ReconfigureCallback = std::function<void(xcb_window_t xid, int16_t x, int16_t y, uint16_t width, uint16_t height)>;

    struct FreeXcbKeySymbols {
        void operator()(xcb_key_symbols_t* symbols) {
            xcb_key_symbols_free(symbols);
        }
    };

    WindowContext& window_context;
    std::unique_ptr<xcb_key_symbols_t, FreeXcbKeySymbols> key_symbols;
    std::unordered_map<xcb_keysym_t, ActionCallback> key_bindings;
    std::function<void()> quit_binding = nullptr;
    ReconfigureCallback reconfigure_binding = nullptr;

public:
    EventLoop(WindowContext& window_context);

    void poll_events();

    template <typename F>
    void bind(xcb_keysym_t key, F f) {
        this->key_bindings[key] = ActionCallback(f);
    }

    template <typename F>
    void bind_quit(F f) {
        this->quit_binding = std::function<void()>(f);
    }

    template <typename F>
    void bind_reconfigure(F f) {
        this->reconfigure_binding = ReconfigureCallback(f);
    }

private:
    void dispatch_event(const xcb_generic_event_t& event);
};

#endif

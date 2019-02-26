#include "interactive/EventLoop.h"
#include <algorithm>

EventLoop::EventLoop(WindowContext& window_context):
    window_context(window_context),
    key_symbols(xcb_key_symbols_alloc(window_context.connection)) {
}

void EventLoop::poll_events() {
    while (true) {
        auto event = std::unique_ptr<xcb_generic_event_t>(
            xcb_poll_for_event(this->window_context.connection)
        );

        if (!event)
            break;

        this->dispatch_event(*event.get());
    }
}

void EventLoop::dispatch_event(const xcb_generic_event_t& event) {
    auto dispatch_key_event = [this](Action action, xcb_keycode_t kc) {
        xcb_keysym_t key = xcb_key_symbols_get_keysym(this->key_symbols.get(), kc, 0);

        auto it = this->key_bindings.find(key);
        if (it != this->key_bindings.end())
            it->second(action);
    };

    switch (static_cast<int>(event.response_type) & ~0x80) {
        case XCB_CONFIGURE_REQUEST: {
            const auto& event_args = reinterpret_cast<const xcb_configure_notify_event_t&>(event);

            if (this->reconfigure_binding) {
                this->reconfigure_binding(event_args.event, event_args.x, event_args.y, event_args.width, event_args.height);
            }

            break;
        }
        case XCB_KEY_PRESS: {
            const auto& event_args = reinterpret_cast<const xcb_key_press_event_t&>(event);
            dispatch_key_event(Action::Press, event_args.detail);
            break;
        }
        case XCB_KEY_RELEASE: {
            const auto& event_args = reinterpret_cast<const xcb_key_release_event_t&>(event);
            dispatch_key_event(Action::Release, event_args.detail);
            break;
        }
        case XCB_CLIENT_MESSAGE: {
            const auto& event_args = reinterpret_cast<const xcb_client_message_event_t&>(event);

            if (event_args.data.data32[0] == this->window_context.atom_wm_delete_window->atom
                && this->quit_binding) {
                this->quit_binding();
            }

            break;
        }
    }   
}

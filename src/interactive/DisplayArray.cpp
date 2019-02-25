#include "interactive/DisplayArray.h"
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <X11/keysym.h>

DisplayArray::DisplayArray(WindowContext& window_context, std::vector<std::unique_ptr<Display>>&& displays):
    window_context(window_context),
    displays(std::move(displays)),
    symbols(xcb_key_symbols_alloc(window_context.connection)),
    close_requested(false) {
}

void DisplayArray::event(const xcb_generic_event_t& event) {
    switch (static_cast<int>(event.response_type) & ~0x80) {
        case XCB_CONFIGURE_REQUEST: {
            const auto& event_args = reinterpret_cast<const xcb_configure_notify_event_t&>(event);
            const xcb_window_t xid = event_args.event;

            const auto it = std::find_if(this->displays.begin(), this->displays.end(), [xid](const auto& display) {
                return display->xid() == xid;
            });

            if (it == this->displays.end())
                break; // Strangely, the display was not one of ours

            auto area = vk::Rect2D(
                {0, 0}, // TODO: add monitor offset
                {
                    static_cast<uint32_t>(event_args.width),
                    static_cast<uint32_t>(event_args.height)
                }
            );

            it->get()->reconfigure(area);
            break;
        }
        case XCB_KEY_PRESS: {
            const auto& event_args = reinterpret_cast<const xcb_key_press_event_t&>(event);
            xcb_keysym_t key = xcb_key_symbols_get_keysym(this->symbols.get(), event_args.detail, 0);

            if (key == XK_Escape) {
                this->close_requested = true;
            }
            break;
        }
        case XCB_CLIENT_MESSAGE: {
            const auto& event_args = reinterpret_cast<const xcb_client_message_event_t&>(event);

            if (this->window_context.is_close_event(event_args)) {
                this->close_requested = true;
            }
            break;
        }
    }
}

void DisplayArray::present() {
    for (auto& display : this->displays) {
        display->present();
    }
}

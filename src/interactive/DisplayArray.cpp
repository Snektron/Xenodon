#include "interactive/DisplayArray.h"
#include <algorithm>
#include <cstdint>
#include <X11/keysym.h>

DisplayArray::DisplayArray(WindowManager& manager, std::vector<Display>&& displays):
    manager(manager),
    displays(std::move(displays)),
    symbols(xcb_key_symbols_alloc(manager.connection)),
    close_requested(false) {
}

void DisplayArray::event(xcb_generic_event_t& event) {
    switch (static_cast<int>(event.response_type) & ~0x80) {
        case XCB_CONFIGURE_REQUEST: {
            const auto& event_args = reinterpret_cast<const xcb_configure_notify_event_t&>(event);
            const xcb_window_t xid = event_args.event;

            const auto it = std::find_if(this->displays.begin(), this->displays.end(), [xid](const Display& display) {
                return display.xid() == xid;
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

            it->reconfigure(area);
        }
        case XCB_KEY_PRESS: {
            const auto& event_args = reinterpret_cast<const xcb_key_press_event_t&>(event);
            xcb_keysym_t key = xcb_key_symbols_get_keysym(this->symbols.get(), event_args.detail, 0);

            if (key == XK_Escape) {
                this->close_requested = true;
            }
        }
        case XCB_CLIENT_MESSAGE: {
            const auto& event_args = reinterpret_cast<const xcb_client_message_event_t&>(event);

            // Check if the [X] button has been pressed
            if (event_args.data.data32[0] == this->manager.delete_window_atom()->atom) {
                this->close_requested = true;
            }
        }
    }
}

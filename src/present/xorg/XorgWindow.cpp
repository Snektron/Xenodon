#include "present/xorg/XorgWindow.h"
#include <stdexcept>
#include "present/xorg/xorg_translate_key.h"
#include <iostream>

// Thank you xcb-xkb devs, very cool!
#define explicit explicit_
#include <xcb/xkb.h>
#undef explicit

namespace {
    constexpr const uint32_t EVENT_MASK =
          XCB_EVENT_MASK_KEY_RELEASE
        | XCB_EVENT_MASK_KEY_PRESS
        | XCB_EVENT_MASK_EXPOSURE
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_POINTER_MOTION
        | XCB_EVENT_MASK_BUTTON_PRESS
        | XCB_EVENT_MASK_BUTTON_RELEASE;

    const char* x_strerr(int err) {
        switch (err) {
            case XCB_CONN_ERROR:
                return "Failed to open X connection: Socket, pipe or stream error";
            case XCB_CONN_CLOSED_EXT_NOTSUPPORTED:
                return "Failed to open X connection: Extension not supported";
            case XCB_CONN_CLOSED_MEM_INSUFFICIENT:
                return "Failed to open X connection: Insufficient memory";
            case XCB_CONN_CLOSED_REQ_LEN_EXCEED:
                return "Failed to open X connection: Exceeded server request length";
            case XCB_CONN_CLOSED_PARSE_ERR:
                return "Failed to open X connection: Display string parse error";
            case XCB_CONN_CLOSED_INVALID_SCREEN:
                return "Failed to open X connection: Display server does not have screen matching display string";
            default:
                return "";
        }
    }
}

XorgWindow::XorgWindow(EventDispatcher& dispatcher, uint16_t width, uint16_t height):
    dispatcher(&dispatcher),
    width(width), height(height) {
    int preferred_screen_index;

    {
        this->connection = XcbConnectionPtr(xcb_connect(nullptr, &preferred_screen_index));

        int err = xcb_connection_has_error(this->connection.get());
        if (err > 0) {
            throw std::runtime_error(x_strerr(err));
        }
    }

    const xcb_setup_t* setup = xcb_get_setup(this->connection.get());
    auto it = xcb_setup_roots_iterator(setup);

    for (int i = 0; i < preferred_screen_index; ++i) {
        xcb_screen_next(&it);
    }

    xcb_screen_t* screen = it.data;

    {
        this->window = xcb_generate_id(this->connection.get());

        uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        uint32_t value_list[] = {
            screen->black_pixel,
            EVENT_MASK
        };

        xcb_create_window_checked(
            this->connection.get(),
            XCB_COPY_FROM_PARENT,
            this->window,
            screen->root,
            0,
            0,
            width,
            height,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            screen->root_visual,
            value_mask,
            value_list
        );

        xcb_map_window(this->connection.get(), this->window);
    }

    {
        this->atom_wm_delete_window = this->atom(false, std::string_view{"WM_DELETE_WINDOW"});

        AtomReply atom_wm_protocols = this->atom(true, std::string_view{"WM_PROTOCOLS"});

        xcb_change_property(
            this->connection.get(),
            XCB_PROP_MODE_REPLACE,
            this->window,
            atom_wm_protocols->atom,
            XCB_ATOM_ATOM,
            32,
            1,
            &this->atom_wm_delete_window->atom
        );
    }

    // Enable detectable key repeat. This will make a repeat appear as a single press 
    // instead of a press and a release
    {
        xcb_xkb_use_extension(this->connection.get(), 1, 0);
        xcb_xkb_per_client_flags(
            this->connection.get(),
            XCB_XKB_ID_USE_CORE_KBD,
            XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT,
            XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT,
            0,0,0
        );
    }

    xcb_flush(this->connection.get());

    this->key_symbols = XcbKeySymbolsPtr(
        xcb_key_symbols_alloc(this->connection.get())
    );
}

XorgWindow::~XorgWindow() {
    if (this->connection.get()) {
        xcb_destroy_window(this->connection.get(), this->window);
    }
}

void XorgWindow::poll_events(XorgSurface& surface) {
    while (true) {
        auto event = MallocPtr<xcb_generic_event_t>(
            xcb_poll_for_event(this->connection.get())
        );

        if (!event)
            break;

        this->handle_event(surface, *event.get());
    }
}

void XorgWindow::handle_event(XorgSurface& surface, const xcb_generic_event_t& event) {
    auto dispatch_key_event = [this](Action action, xcb_keycode_t kc) {
        xcb_keysym_t keysym = xcb_key_symbols_get_keysym(this->key_symbols.get(), kc, 0);
        Key key = xorg_translate_key(keysym);
        this->dispatcher->dispatch_key_event(key, action);
    };

    switch (static_cast<int>(event.response_type) & ~0x80) {
        case XCB_CLIENT_MESSAGE: {
            const auto& event_args = reinterpret_cast<const xcb_client_message_event_t&>(event);

            if (event_args.data.data32[0] == this->atom_wm_delete_window->atom) {
                this->dispatcher->dispatch_close_event();
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
        case XCB_CONFIGURE_NOTIFY: {
            const auto& event_args = reinterpret_cast<const xcb_configure_notify_event_t&>(event);

            // Is the notification a window drag event?
            if (this->width == event_args.width && this->height == event_args.height)
                break;

            // TODO: notify the surface of change

            this->width = event_args.width;
            this->height = event_args.height;

            this->dispatcher->dispatch_resize_event(event_args.width, event_args.height);

            break;
        }
    }
}

std::pair<xcb_connection_t*, xcb_window_t> XorgWindow::x_handles() {
    return {
        this->connection.get(),
        this->window
    };
}

XorgWindow::AtomReply XorgWindow::atom(bool only_if_exists, const std::string_view& str) const {
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(
        this->connection.get(),
        uint8_t(only_if_exists),
        static_cast<uint16_t>(str.size()),
        str.data()
    );

    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(this->connection.get(), cookie, nullptr);
    return AtomReply(reply);
}

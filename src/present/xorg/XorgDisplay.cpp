#include "present/xorg/XorgDisplay.h"
#include <string_view>
#include <stdexcept>
#include <utility>
#include <iostream>

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

    xcb_connection_t* connect_checked(int* preferred_screen_index) {
        xcb_connection_t* connection = xcb_connect(nullptr, preferred_screen_index);

        int err = xcb_connection_has_error(connection);
        if (err > 0) {
            throw std::runtime_error(x_strerr(err));
        }

        return connection;
    }
}

XorgDisplay::XorgDisplay(vk::Instance instance, EventDispatcher& dispatcher, uint16_t width, uint16_t height):
    instance(instance), dispatcher(&dispatcher),
    connection(connect_checked(&this->preferred_screen_index)),
    atom_wm_delete_window(this->atom(false, std::string_view{"WM_DELETE_WINDOW"})),
    window(xcb_generate_id(this->connection)),
    kbd(this->connection) {

    // Get the preferred X screen, to properly handle the $DISPLAY
    // environment variable
    const xcb_setup_t* setup = xcb_get_setup(this->connection);
    auto it = xcb_setup_roots_iterator(setup);

    for (int i = 0; i < this->preferred_screen_index; ++i) {
        xcb_screen_next(&it);
    }

    xcb_screen_t* screen = it.data;

    // Initialize the X window
    {
        uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        uint32_t value_list[] = {
            screen->black_pixel,
            EVENT_MASK
        };

        xcb_create_window_checked(
            this->connection,
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

        xcb_map_window(this->connection, this->window);
    }

    // Enable window delete events
    {
        AtomReply atom_wm_protocols = this->atom(true, std::string_view{"WM_PROTOCOLS"});

        xcb_change_property(
            this->connection,
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
        xcb_xkb_use_extension(connection, 1, 0);
        xcb_xkb_per_client_flags(
            this->connection,
            XCB_XKB_ID_USE_CORE_KBD,
            XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT,
            XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT,
            0,0,0
        );
    }

    xcb_flush(this->connection);
}

XorgDisplay::XorgDisplay(XorgDisplay&& other):
    instance(other.instance),
    dispatcher(other.dispatcher),
    connection(other.connection),
    preferred_screen_index(other.preferred_screen_index),
    atom_wm_delete_window(std::move(other.atom_wm_delete_window)),
    window(other.window),
    kbd(std::move(other.kbd)) {
    other.connection = nullptr;
}

XorgDisplay& XorgDisplay::operator=(XorgDisplay&& other) {
    this->instance = other.instance;
    this->dispatcher = other.dispatcher;

    std::swap(this->connection, other.connection);
    std::swap(this->preferred_screen_index, other.preferred_screen_index);
    std::swap(this->atom_wm_delete_window, other.atom_wm_delete_window);
    std::swap(this->window, other.window);
    std::swap(this->kbd, other.kbd);
    return *this;
}

XorgDisplay::~XorgDisplay() {
    if (this->connection) {
        xcb_destroy_window(this->connection, this->window);
        xcb_disconnect(this->connection);
    }
}

void XorgDisplay::poll_events() {
    while (true) {
        auto event = MallocPtr<xcb_generic_event_t>(
            xcb_poll_for_event(this->connection)
        );

        if (!event)
            break;

        this->handle_event(*event.get());
    }
}

void XorgDisplay::handle_event(const xcb_generic_event_t& event) {
    auto dispatch_key_event = [this](Action action, xcb_keycode_t kc) {
        Key key = this->kbd.translate(kc);
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
    }
}

XorgDisplay::AtomReply XorgDisplay::atom(bool only_if_exists, const std::string_view& str) const {
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(
        this->connection,
        uint8_t(only_if_exists),
        static_cast<uint16_t>(str.size()),
        str.data()
    );

    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(this->connection, cookie, nullptr);
    return AtomReply(reply);
}

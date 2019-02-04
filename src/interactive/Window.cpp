#include "interactive/Window.h"
#include <string_view>
#include <cstdint>

namespace {
    constexpr const uint32_t EVENT_MASK =
          XCB_EVENT_MASK_KEY_RELEASE
        | XCB_EVENT_MASK_KEY_PRESS
        | XCB_EVENT_MASK_EXPOSURE
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_POINTER_MOTION
        | XCB_EVENT_MASK_BUTTON_PRESS
        | XCB_EVENT_MASK_BUTTON_RELEASE;

    Window::AtomReply atom(xcb_connection_t* connection, bool only_if_exists, const std::string_view& str) {
        xcb_intern_atom_cookie_t cookie = xcb_intern_atom(
            connection,
            only_if_exists,
            static_cast<uint16_t>(str.size()),
            str.data()
        );

        xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(connection, cookie, NULL);
        return Window::AtomReply(reply);
    }

    using std::literals::operator ""sv;
}

Window::Window(xcb_connection_t* connection, xcb_screen_t* screen):
    connection(connection),
    screen(screen),
    window(xcb_generate_id(connection)),
    atom_wm_delete_window(atom(connection, false, "WM_DELETE_WINDOW"sv)),
    width(screen->width_in_pixels),
    height(screen->height_in_pixels) {
    uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t value_list[] = {
        this->screen->black_pixel,
        EVENT_MASK
    };

    xcb_create_window(
        this->connection,
        XCB_COPY_FROM_PARENT,
        this->window,
        screen->root,
        0,
        0,
        this->width,
        this->height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        value_mask,
        value_list
    );

    // Set window full screen
    {
        AtomReply atom_wm_state = atom(connection, false, "_NET_WM_STATE"sv);
        AtomReply atom_wm_fullscreen = atom(connection, false, "_NET_WM_STATE_FULLSCREEN"sv);
        xcb_change_property(
            this->connection,
            XCB_PROP_MODE_REPLACE,
            this->window,
            atom_wm_state->atom,
            XCB_ATOM_ATOM,
            32,
            1,
            &atom_wm_fullscreen->atom
        );
    }

    // Make window send destroy notifications
    {
        AtomReply atom_wm_protocols = atom(connection, true, "WM_PROTOCOLS"sv);

        xcb_change_property(
            this->connection,
            XCB_PROP_MODE_REPLACE,
            this->window,
            atom_wm_protocols->atom,
            4,
            32,
            1,
            &this->atom_wm_delete_window->atom
        );
    }
}

Window::~Window() {
    xcb_destroy_window(this->connection, this->window);
}

void Window::set_title(const std::string_view& title) const {
    xcb_change_property(
        this->connection,
        XCB_PROP_MODE_REPLACE,
        this->window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        static_cast<uint32_t>(title.size()),
        title.data()
    );
}

void Window::map() const {
    xcb_map_window(this->connection, this->window);
}

void Window::unmap() const {
    xcb_unmap_window(this->connection, this->window);
}
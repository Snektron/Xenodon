#include "interactive/WindowManager.h"

namespace {
    using std::literals::operator ""sv;
}

WindowManager::WindowManager(xcb_connection_t* connection):
    connection(connection),
    atom_wm_delete_window(this->atom(false, "WM_DELETE_WINDOW"sv)) {
}

WindowManager::AtomReply WindowManager::atom(bool only_if_exists, const std::string_view& str) const {
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(
        this->connection,
        only_if_exists,
        static_cast<uint16_t>(str.size()),
        str.data()
    );

    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(this->connection, cookie, nullptr);
    return WindowManager::AtomReply(reply);
}
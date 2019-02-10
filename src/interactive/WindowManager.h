#ifndef _XENODON_INTERACTIVE_WINDOWMANAGER_H
#define _XENODON_INTERACTIVE_WINDOWMANAGER_H

#include <memory>
#include <string_view>
#include <xcb/xcb.h>
#include "utility/MallocPtr.h"

class WindowManager {
public:
    using AtomReply = MallocPtr<xcb_intern_atom_reply_t>;

    xcb_connection_t* connection;
    AtomReply atom_wm_delete_window;

    WindowManager(xcb_connection_t* connection);

    xcb_intern_atom_reply_t* delete_window_atom() {
        return this->atom_wm_delete_window.get();
    }

private:
    AtomReply atom(bool only_if_exists, const std::string_view& str) const;

    friend class DisplayArray;
};

#endif

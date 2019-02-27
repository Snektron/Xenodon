#ifndef _XENODON_PRESENT_XORG_KEYBOARD_H
#define _XENODON_PRESENT_XORG_KEYBOARD_H

#include <memory>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include "present/Key.h"

class Keyboard {
    struct FreeXcbKeySymbols {
        void operator()(xcb_key_symbols_t* symbols) {
            xcb_key_symbols_free(symbols);
        }
    };

    std::unique_ptr<xcb_key_symbols_t, FreeXcbKeySymbols> key_symbols;

public:
    Keyboard(xcb_connection_t* connection);
    Key translate(xcb_keycode_t kc);
};

#endif

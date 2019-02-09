#ifndef _XENODON_INTERACTIVE_DISPLAYARRAY_H
#define _XENODON_INTERACTIVE_DISPLAYARRAY_H

#include <vector>
#include <memory>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include "interactive/Display.h"
#include "interactive/WindowManager.h"

class DisplayArray {
    struct FreeXcbKeySymbols {
        void operator()(xcb_key_symbols_t* symbols) {
            xcb_key_symbols_free(symbols);
        }
    };

    WindowManager& manager;
    std::vector<Display> displays;
    std::unique_ptr<xcb_key_symbols_t, FreeXcbKeySymbols> symbols;

public:
    bool close_requested;

    explicit DisplayArray(WindowManager& manager, std::vector<Display>&& displays);
    void event(xcb_generic_event_t& event);
};

#endif

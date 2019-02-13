#ifndef _XENODON_INTERACTIVE_DISPLAYARRAY_H
#define _XENODON_INTERACTIVE_DISPLAYARRAY_H

#include <vector>
#include <memory>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include "interactive/Display.h"
#include "interactive/Window.h"

class DisplayArray {
    struct FreeXcbKeySymbols {
        void operator()(xcb_key_symbols_t* symbols) {
            xcb_key_symbols_free(symbols);
        }
    };

    WindowContext& window_context;
    std::vector<std::unique_ptr<Display>> displays;
    std::unique_ptr<xcb_key_symbols_t, FreeXcbKeySymbols> symbols;

public:
    bool close_requested;

    DisplayArray(WindowContext& window_context, std::vector<std::unique_ptr<Display>>&& displays);
    void event(const xcb_generic_event_t& event);
    void render();
};

#endif

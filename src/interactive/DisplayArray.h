#ifndef _XENODON_INTERACTIVE_DISPLAYARRAY_H
#define _XENODON_INTERACTIVE_DISPLAYARRAY_H

#include <vector>
#include <cstdint>
#include <xcb/xcb.h>
#include "interactive/Display.h"

class DisplayArray {
    std::vector<std::unique_ptr<Display>> displays;

public:
    DisplayArray(std::vector<std::unique_ptr<Display>>&& displays);
    void reconfigure(xcb_window_t xid, int16_t x, int16_t y, uint16_t width, uint16_t height);
    void present();
};

#endif

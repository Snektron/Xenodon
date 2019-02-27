#ifndef _XENODON_PRESENT_DISPLAY_H
#define _XENODON_PRESENT_DISPLAY_H

class Display {
public:
    virtual void poll_events() = 0;
    virtual ~Display() = default;
};

#endif

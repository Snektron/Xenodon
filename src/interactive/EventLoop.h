#ifndef _XENODON_INTERACTIVE_EVENTHANDLER_H
#define _XENODON_INTERACTIVE_EVENTHANDLER_H

#include <unordered_map>
#include <xcb/xcb.h>
#include "utility/MallocPtr.h"

class EventLoop {
    xcb_connection_t* connection;

public:
    EventLoop(xcb_connection_t* connection);
    MallocPtr<xcb_generic_event_t> poll_event();
};

#endif

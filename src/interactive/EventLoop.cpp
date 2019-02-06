#include "interactive/EventLoop.h"
#include <utility>

EventLoop::EventLoop(xcb_connection_t* connection):
    connection(connection) {
}

MallocPtr<xcb_generic_event_t> EventLoop::poll_event() {
    xcb_generic_event_t* event = xcb_poll_for_event(connection);
    return MallocPtr<xcb_generic_event_t>(event);
}
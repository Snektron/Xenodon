#include <iostream>
#include <cstddef>
#include <string_view>
#include "headless/headless.h"
#include "interactive/interactive.h"
#include "resources.h"

#include <stdexcept>
#include <xcb/xcb.h>
#include "interactive/Window.h"
#include "utility/ScopeGuard.h"

const char* event_name(int type) {
    switch (type) {
        case XCB_KEY_PRESS:
            return "XCB_KEY_PRESS";
        case XCB_KEY_RELEASE:
            return "XCB_KEY_RELEASE";
        case XCB_BUTTON_PRESS:
            return "XCB_BUTTON_PRESS";
        case XCB_BUTTON_RELEASE:
            return "XCB_BUTTON_RELEASE";
        case XCB_MOTION_NOTIFY:
            return "XCB_MOTION_NOTIFY";
        case XCB_ENTER_NOTIFY:
            return "XCB_ENTER_NOTIFY";
        case XCB_LEAVE_NOTIFY:
            return "XCB_LEAVE_NOTIFY";
        case XCB_FOCUS_IN:
            return "XCB_FOCUS_IN";
        case XCB_FOCUS_OUT:
            return "XCB_FOCUS_OUT";
        case XCB_EXPOSE:
            return "XCB_EXPOSE";
        case XCB_GRAPHICS_EXPOSURE:
            return "XCB_GRAPHICS_EXPOSURE";
        case XCB_NO_EXPOSURE:
            return "XCB_NO_EXPOSURE";
        default:
            return "Other event type";
    }
}

void test_xcb() {
    xcb_connection_t* connection = xcb_connect(NULL, NULL);
    auto _ = ScopeGuard([connection]{
        xcb_disconnect(connection);
    });

    for (xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(connection)); it.rem; xcb_screen_next(&it)) {
        xcb_screen_t* screen = it.data;
        std::cout << "Screen " << screen->width_in_pixels << "x" << screen->height_in_pixels << std::endl;
    }

    xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

    auto win = Window(connection, screen);

    xcb_generic_event_t* event;

    while ((event = xcb_wait_for_event(connection))) {
        int type = static_cast<int>(event->response_type) & ~0x80;
        free(event);
        std::cout << "event " << event_name(type) << " (" << type << ')' << std::endl;
        if (type == XCB_KEY_PRESS)
            return;
    }
}

void print_help(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n\n"
        << resources::open("resources/options.txt") << std::endl;
}

int main(int argc, char* argv[]) {
    bool interactive = true;

    for (size_t i = 1; i < static_cast<size_t>(argc); ++i) {
        auto arg = std::string_view(argv[i]);
        if (arg == "--headless") {
            interactive = false;
        } else if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else {
            std::cerr << "Invalid argument '" << arg << "'.\nUse "
                << argv[0] << " --help for more information on usage." << std::endl;
            return 0;
        }
    }

    if (interactive)
        interactive_main();
    else
        headless_main();
}
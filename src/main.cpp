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

void test_xcb() {
    xcb_connection_t* connection = xcb_connect(NULL, NULL);
    if (!connection) {
        throw std::runtime_error("Failed to initialize XCB connection");
    }

    auto _ = ScopeGuard([connection]{
        xcb_disconnect(connection);
    });

    xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    auto win = Window(connection, screen);
    win.map();

    xcb_flush(connection);

    xcb_generic_event_t* event;

    while ((event = xcb_wait_for_event(connection))) {
        int type = static_cast<int>(event->response_type) & ~0x80;
        free(event);
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

    test_xcb();

    // if (interactive)
    //     interactive_main();
    // else
    //     headless_main();
}
#include <iostream>
#include <cstddef>
#include <string_view>
#include "headless/headless.h"
#include "interactive/interactive.h"
#include "resources.h"

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
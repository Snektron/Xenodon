#include <iostream>
#include <cstddef>
#include <string_view>
#include "headless/headless.h"
#include "interactive/interactive.h"
#include "resources.h"

void print_help(const std::string_view program_name) {
    std::cout << "Usage: " << program_name << " [options]\n\n"
        << resources::open("resources/options.txt") << std::endl;
}

int main(int argc, char* argv[]) {
    bool interactive = true;
    auto program_name = std::string_view(argv[0]);

    for (size_t i = 1; i < static_cast<size_t>(argc); ++i) {
        auto arg = std::string_view(argv[i]);
        if (arg == "--headless") {
            interactive = false;
        } else if (arg == "-h" || arg == "--help") {
            print_help(program_name);
        } else {
            std::cerr << "Invalid argument '" << arg << "'.\nUse "
                << program_name << " --help for more information on usage" << std::endl;
            return 0;
        }
    }

    if (interactive)
        interactive_main();
    else
        headless_main();
}
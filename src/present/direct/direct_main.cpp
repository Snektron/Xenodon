#include "present/direct/direct_main.h"
#include <iostream>
#include <fstream>
#include "present/direct/DisplayConfig.h"
#include "main_loop.h"

void direct_main(int argc, char* argv[]) {
    if (argc == 0) {
        std::cout << "Error: Missing argument <display config>" << std::endl;
        return;
    }

    auto in = std::ifstream(argv[0]);
    if (!in) {
        std::cout << "Error: Failed to open display config file '" << argv[0] << '\'' << std::endl;
        return;
    }

    try {
        auto config = DisplayConfig(in);
    } catch (const DisplayConfig::ParseError& err) {
        std::cout << "Error: Failed to parse config file '" << argv[0] << "':\n"
            << err.what() << std::endl;
        return;
    }
}

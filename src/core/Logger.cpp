#include "core/Logger.h"
#include <iostream>
#include <string>
#include <ctime>
#include <fmt/time.h>

void ConsoleSink::write(std::string_view line) {
    std::cout << line << '\n';
}

FileSink::FileSink(std::string_view path):
    file(std::string(path).c_str()) {
}

void FileSink::write(std::string_view line) {
    this->file << line << '\n';
}

void Logger::write(std::string_view fmt, fmt::format_args args) {
    auto buf = fmt::memory_buffer();
    std::time_t t = std::time(nullptr);
    fmt::format_to(buf, "[{:%H:%M:%S} {}] ", *std::localtime(&t), this->frame);
    fmt::vformat_to(buf, fmt, args);
    
    auto msg = fmt::to_string(buf);
    for (auto& sink : this->sinks) {
        sink->write(msg);
    }
}

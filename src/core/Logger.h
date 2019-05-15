#ifndef _XENODON_CORE_LOGGER_H
#define _XENODON_CORE_LOGGER_H

#include <string_view>
#include <memory>
#include <utility>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstddef>
#include <fmt/format.h>

struct Sink {
    virtual ~Sink() = default;
    virtual void write(std::string_view line) = 0;
};

struct ConsoleSink final: public Sink {
    ConsoleSink() = default;
    void write(std::string_view line) override;
};

class FileSink final: public Sink {
    std::ofstream file;

public:
    FileSink(std::filesystem::path path);
    void write(std::string_view line) override;
};

class Logger {
    std::vector<std::unique_ptr<Sink>> sinks;

public:
    Logger() = default;

    template <typename T, typename... Args>
    void add_sink(Args&&... args);

    template <typename... Args>
    void log(std::string_view fmt, const Args&... args);

private:
    void write(std::string_view fmt, fmt::format_args args);
};

template <typename T, typename... Args>
void Logger::add_sink(Args&&... args) {
    this->sinks.push_back(std::make_unique<T>(std::forward<Args>(args)...));
}

template <typename... Args>
void Logger::log(std::string_view fmt, const Args&... args) {
    this->write(fmt, fmt::make_format_args(args...));
}

extern Logger LOGGER;

#endif

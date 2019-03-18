#ifndef _XENODON_CONFIG_CONFIG_H
#define _XENODON_CONFIG_CONFIG_H

#include <istream>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <string>
#include <sstream>
#include <tuple>
#include <array>
#include <optional>
#include <iostream>
#include <cstddef>

struct ParseError: public std::runtime_error {
    ParseError(const std::string& msg):
        runtime_error(msg) {
    }
};

class Parser {
    std::istream& input;
    size_t line;
    size_t column;

public:
    Parser(std::istream& input);

    int peek();
    int consume();
    void expect(int expected);

    template <typename F>
    std::string fmt_error(F f) {
        auto ss = std::stringstream();
        ss << "Parse error at " << this->line << ", " << this->column << ": ";
        f(ss);
        return ss.str();
    }

    std::string error(std::string_view msg) {
        return this->fmt_error([msg](auto& ss){ ss << msg; });
    }

    std::string parse_key();
};

template <typename T>
struct Accumulator;

template <typename T>
struct Value {
    using Output = T;

    std::string_view key;

    Value(std::string_view key):
        key(key) {
    }
};

template <typename T>
struct Accumulator<Value<T>> {
    std::optional<T> value;

    void operator()(const T& value) {
        if (this->value) {
            throw std::runtime_error("Ambiguous key");
        } else {
            this->value = value;
        }
    }

    T get() const {
        if (this->value) {
            return this->value.value();
        } else {
            throw std::runtime_error("Missing key");
        }
    }
};

template <typename T>
struct Struct {
    using Output = T;

    std::string_view key;

    Struct(std::string_view key):
        key(key) {
    }
};

template <typename T>
struct Accumulator<Struct<T>> {
    std::optional<T> value;

    void operator()(const T& value) {
        if (this->value) {
            throw std::runtime_error("Ambiguous key");
        } else {
            this->value = value;
        }
    }

    T get() const {
        if (this->value) {
            return this->value.value();
        } else {
            throw std::runtime_error("Missing key");
        }
    }
};

template <typename T>
struct Vector {
    using Output = std::vector<T>;

    std::string_view key;

    Vector(std::string_view key):
        key(key) {
    }
};

template <typename T>
struct Accumulator<Vector<T>> {
    std::vector<T> values;

    void operator()(const T& value) {
        this->values.push_back(value);
    }

    std::vector<T> get() {
        return std::move(this->values);
    }
};

class Config {
    Parser parser;

public:
    Config(std::istream& input):
        parser(input) {
    }

    template <typename... Items>
    std::tuple<typename Items::Output...> get(Items&&... items) {
        return this->parse('}', std::index_sequence_for<Items...>{}, std::forward<Items>(items)...);
    }

    template <typename... Items>
    std::tuple<typename Items::Output...> root(Items&&... items) {
        return this->parse(-1, std::index_sequence_for<Items...>{}, std::forward<Items>(items)...);
    }

private:
    template <size_t... Indices, typename... Items>
    std::tuple<typename Items::Output...> parse(int delim, std::index_sequence<Indices...>, Items&&... items) {
        auto accumulators = std::tuple<Accumulator<Items>...>{};

        auto parse_item = [&](std::string_view key, auto item, auto& accum) -> bool {
            if (key == item.key) {
                std::cout << "We got one, boss: " << key << std::endl;
                return true;
            }
            return false;
        };

        int c = this->parser.peek();
        while (c != delim) {
            std::string key = this->parser.parse_key();

            bool parsed = (parse_item(key, items, std::get<Indices>(accumulators)) && ...);
            if (!parsed) {
                throw ParseError(parser.fmt_error([&key](auto& ss) {
                    ss << "Unexpected key '" << key << '\'';
                }));
            }

            c = this->parser.peek();
        }

        return std::tuple(std::get<Indices>(accumulators).get()...);
    }
};

#endif

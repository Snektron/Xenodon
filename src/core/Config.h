#ifndef _XENODON_CORE_CONFIG_H
#define _XENODON_CORE_CONFIG_H

#include <istream>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <tuple>
#include <array>
#include <optional>
#include <cstddef>
#include <cctype>
#include <fmt/format.h>

namespace cfg {
    struct ConfigError: public std::runtime_error {
        template <typename... Args>
        std::string format_error(std::string_view fmt, const Args&... args) {
            auto buf = fmt::memory_buffer();
            fmt::format_to(buf, "Configuration error: ");
            fmt::format_to(buf, fmt, args...);
            return fmt::to_string(buf);
        }

        template <typename... Args>
        ConfigError(std::string_view fmt, const Args&... args):
            runtime_error(format_error(fmt, args...)) {
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
        void optws();
        void expectws();

        std::string parse_key();

        friend struct ParseError;
    };

    struct ParseError: public std::runtime_error {
        template <typename... Args>
        std::string format_error(const Parser& parser, std::string_view fmt, const Args&... args) {
            auto buf = fmt::memory_buffer();
            fmt::format_to(buf, "Parse error at {}, {}: ", parser.line, parser.column);
            fmt::format_to(buf, fmt, args...);
            return fmt::to_string(buf);
        }

        template <typename... Args>
        ParseError(const Parser& parser, std::string_view fmt, const Args&... args):
            runtime_error(format_error(parser, fmt, args...)) {
        }
    };

    template <typename T>
    struct Parse;

    template <typename T>
    struct FromConfig;

    template <typename T>
    struct Accumulator;

    class Config;

    template <typename T>
    struct Value {
        using Output = T;

        std::string_view key;

        Value(std::string_view key):
            key(key) {
        }

        T parse(Config& cfg) const;
    };

    template <typename T>
    struct Accumulator<Value<T>> {
        std::optional<T> value;

        void operator()(std::string_view key, const T& value) {
            if (this->value) {
                throw ConfigError("Ambiguous key '{}'", key);
            } else {
                this->value = value;
            }
        }

        T get(std::string_view key) const {
            if (this->value) {
                return this->value.value();
            } else {
                throw ConfigError("Missing key '{}'", key);
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

        T parse(Config& cfg) const;
    };

    template <typename T>
    struct Accumulator<Struct<T>> {
        std::optional<T> value;

        void operator()(std::string_view key, const T& value) {
            if (this->value) {
                throw ConfigError("Ambiguous key '{}'", key);
            } else {
                this->value = value;
            }
        }

        T get(std::string_view key) const {
            if (this->value) {
                return this->value.value();
            } else {
                throw ConfigError("Missing key '{}'", key);
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

        T parse(Config& cfg) const;
    };

    template <typename T>
    struct Accumulator<Vector<T>> {
        std::vector<T> values;

        void operator()(std::string_view, const T& value) {
            this->values.push_back(value);
        }

        std::vector<T> get(std::string_view) {
            return std::move(this->values);
        }
    };

    class Config {
        Parser parser;

    public:
        Config(std::istream& input):
            parser(input) {
        }

        template <typename T>
        T as() {
            auto result = FromConfig<T>{}(*this);
            int c = this->parser.peek();
            if (c != -1) {
                throw ParseError(parser, "Expected end of input, found '{}'", static_cast<char>(c));
            }
            return result;
        }

        template <typename... Items>
        std::tuple<typename Items::Output...> get(Items&&... items) {
            return this->parse(std::index_sequence_for<Items...>{}, std::forward<Items>(items)...);
        }

        template <typename... Items>
        std::tuple<typename Items::Output...> root(Items&&... items) {
            auto result = this->parse(std::index_sequence_for<Items...>{}, std::forward<Items>(items)...);
            int c = this->parser.peek();
            if (c != -1) {
                throw ParseError(parser, "Expected end of input, found '{}'", static_cast<char>(c));
            }
            return result;
        }

    private:
        template <size_t... Indices, typename... Items>
        std::tuple<typename Items::Output...> parse(std::index_sequence<Indices...>, Items&&... items) {
            auto accumulators = std::tuple<Accumulator<Items>...>{};

            auto parse_item = [&](std::string_view key, const auto& item, auto& accum) -> bool {
                if (key != item.key) {
                    return false;
                }

                accum(item.key, item.parse(*this));

                return true;
            };

            this->parser.optws();
            int c = this->parser.peek();
            while (std::isalpha(c)) {
                std::string key = this->parser.parse_key();
                this->parser.optws();

                bool parsed = (parse_item(key, items, std::get<Indices>(accumulators)) || ...);
                if (!parsed) {
                    throw ParseError(parser, "Unexpected key '{}'", key);
                }

                c = this->parser.peek();
                if (!std::isspace(c)) {
                    break;
                }

                this->parser.optws();
                c = this->parser.peek();
            }

            this->parser.optws();

            return std::tuple(std::get<Indices>(accumulators).get(items.key)...);
        }

        template <typename T>
        friend struct Value;

        template <typename T>
        friend struct Struct;

        template <typename T>
        friend struct Vector;
    };

    template <typename T>
    T Value<T>::parse(Config& cfg) const {
        cfg.parser.expect('=');
        cfg.parser.optws();
        return Parse<T>{}(cfg.parser);
    }

    template <typename T>
    T Struct<T>::parse(Config& cfg) const {
        cfg.parser.expect('{');
        auto result = FromConfig<T>{}(cfg);
        cfg.parser.expect('}');
        return result;
    }

    template <typename T>
    T Vector<T>::parse(Config& cfg) const {
        cfg.parser.expect('{');
        auto result = FromConfig<T>{}(cfg);
        cfg.parser.expect('}');
        return result;
    }

    template<>
    struct Parse<size_t> {
        size_t operator()(Parser& p);
    };

    template<>
    struct Parse<std::string> {
        std::string operator()(Parser& p);
    };
}

#endif

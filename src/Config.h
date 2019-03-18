#ifndef _XENODON_CONFIG_H
#define _XENODON_CONFIG_H

#include <istream>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <string>
#include <sstream>
#include <tuple>
#include <array>
#include <optional>
#include <cstddef>
#include <cctype>

namespace cfg {
    struct ParseError: public std::runtime_error {
        ParseError(const std::string& msg):
            runtime_error(msg) {
        }
    };

    struct ConfigError: public std::runtime_error {
        ConfigError(const std::string& msg):
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
        void optws();
        void expectws();

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
                auto ss = std::stringstream();
                ss << "Config error: Ambiguous key '" << key << '\'';
                throw ConfigError(ss.str());
            } else {
                this->value = value;
            }
        }

        T get(std::string_view key) const {
            if (this->value) {
                return this->value.value();
            } else {
                auto ss = std::stringstream();
                ss << "Config error: Missing key '" << key << '\'';
                throw ConfigError(ss.str());
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
                auto ss = std::stringstream();
                ss << "Config error: Ambiguous key '" << key << '\'';
                throw ConfigError(ss.str());
            } else {
                this->value = value;
            }
        }

        T get(std::string_view key) const {
            if (this->value) {
                return this->value.value();
            } else {
                auto ss = std::stringstream();
                ss << "Config error: Missing key '" << key << '\'';
                throw ConfigError(ss.str());
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

        template <typename... Items>
        std::tuple<typename Items::Output...> get(Items&&... items) {
            return this->parse(std::index_sequence_for<Items...>{}, std::forward<Items>(items)...);
        }

        template <typename... Items>
        std::tuple<typename Items::Output...> root(Items&&... items) {
            auto result = this->parse(std::index_sequence_for<Items...>{}, std::forward<Items>(items)...);
            int c = this->parser.peek();
            if (c != -1) {
                throw ParseError(this->parser.fmt_error([c](auto& ss) {
                    ss << "Expected end of input, found '" << static_cast<char>(c) << "'";
                }));
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
                    throw ParseError(parser.fmt_error([&key](auto& ss) {
                        ss << "Unexpected key '" << key << '\'';
                    }));
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

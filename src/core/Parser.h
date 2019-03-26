#ifndef _XENODON_CORE_PARSE_H
#define _XENODON_CORE_PARSE_H

#include <istream>
#include <string_view>
#include <string>
#include <sstream>
#include <utility>
#include <fmt/format.h>
#include <vulkan/vulkan.hpp>

namespace parser {
    class Parser {
        std::istream& input;
        size_t line;
        size_t column;
        bool multiline;

    public:
        Parser(std::istream& input, bool multiline = false);

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
        ParseError(const Parser& parser, std::string_view fmt, const Args&... args):
            runtime_error(format_error(parser, fmt, fmt::make_format_args(args...))) {
        }

    private:
        std::string format_error(const Parser& parser, std::string_view fmt, fmt::format_args args);
    };

    template <typename T>
    struct Parse;

    template <>
    struct Parse<size_t> {
        size_t operator()(Parser& p) const;
    };

    template <>
    struct Parse<std::string> {
        std::string operator()(Parser& p) const;
    };

    template <typename T, typename U>
    struct parser::Parse<std::pair<T, U>> {
        std::pair<T, U> operator()(parser::Parser& p) const;
    };

    template<>
    struct Parse<vk::Offset2D> {
        vk::Offset2D operator()(Parser& p) const;
    };

    template<>
    struct Parse<vk::Extent2D> {
        vk::Extent2D operator()(Parser& p) const;
    };

    template <typename T>
    T parse(std::istream& stream, bool multiline = false) {
        auto parser = Parser(stream, multiline);
        auto result = Parse<T>{}(parser);
        int c = parser.peek();
        if (c > 0) {
            throw parser::ParseError(parser, "Expected end of input, found '{}'", static_cast<char>(c));
        }
        return result;
    }

    template <typename T>
    T parse(std::string_view str, bool multiline = false) {
        auto ss = std::stringstream();
        ss << str;
        return parse<T>(ss, multiline);
    }

    template <typename T, typename U>
    std::pair<T, U> Parse<std::pair<T, U>>::operator()(parser::Parser& p) const {
        p.expect('(');
        p.optws();
        T x = parser::Parse<T>{}(p);
        p.optws();
        p.expect(',');
        p.optws();
        U y = parser::Parse<U>{}(p);
        p.optws();
        p.expect(')');

        return {x, y};
    }
}

#endif

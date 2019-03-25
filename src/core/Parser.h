#ifndef _XENODON_CORE_PARSE_H
#define _XENODON_CORE_PARSE_H

#include <istream>
#include <string_view>
#include <string>
#include <sstream>
#include <fmt/format.h>

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

    template<>
    struct Parse<size_t> {
        size_t operator()(Parser& p);
    };

    template<>
    struct Parse<std::string> {
        std::string operator()(Parser& p);
    };

    template <typename T>
    T parse(std::istream& stream, bool multiline = false) {
        auto parser = Parser(stream, multiline);
        return Parse<T>{}(parser);
    }

    template <typename T>
    T parse(std::string_view str, bool multiline = false) {
        auto ss = std::stringstream();
        ss << str;
        return parse<T>(ss, multiline);
    }
}

#endif

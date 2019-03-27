#include "core/Parser.h"
#include <cctype>

namespace parser {
    Parser::Parser(std::istream& input, bool multiline):
        input(input),
        line(1),
        column(1),
        multiline(multiline) {
    }

    int Parser::peek() {
        int c = this->input.peek();

        // skip comments
        if (c == '#') {
            if (this->multiline) {
                throw ParseError(*this, "Unexpected character '#'");
            }

            while (c != '\n' && c > 0) {
                this->input.get();
                c = this->input.peek();
            }
        }

        return c;
    }

    int Parser::consume() {
        if (this->input.eof()) {
            throw ParseError(*this, "Unexpected end of input");
        }

        int c = this->input.get();
        if (c == '\n') {
            if (this->multiline) {
                ++this->line;
                this->column = 0;
            } else {
                throw ParseError(*this, "Unexpected newline");
            }
        }

        ++this->column;

        return c;
    }

    void Parser::expect(int expected) {
        int actual = this->consume();
        if (actual != expected) {
            throw ParseError(
                *this,
                "Expected character '{}', found '{}'",
                static_cast<char>(expected),
                static_cast<char>(actual)
            );
        }
    }

    void Parser::optws() {
        while (std::isspace(this->peek())) {
            this->consume();
        }
    }

    void Parser::expectws() {
        int c = this->peek();
        if (!std::isspace(c)) {
            throw ParseError(*this, "Expected whitespace character, found '{}'", static_cast<char>(c));
        }

        this->optws();
    }

    std::string Parser::parse_key() {
        int c = this->consume();

        if (!std::isalpha(c)) {
            throw ParseError(*this, "Expected alphabetic key, found '{}'", static_cast<char>(c));
        }

        auto key = std::stringstream();
        key << static_cast<char>(c);
        while (std::isalpha(this->peek())) {
            key << static_cast<char>(this->consume());
        }

        return key.str();
    }

    std::string ParseError::format_error(const Parser& parser, std::string_view fmt, fmt::format_args args) {
        auto buf = fmt::memory_buffer();
        if (parser.multiline) {
            fmt::format_to(buf, "Parse error at line {}, col {}: ", parser.line, parser.column);
        } else {
            fmt::format_to(buf, "Parse error at col {}: ", parser.column);
        }
        fmt::vformat_to(buf, fmt, args);
        return fmt::to_string(buf);
    }

    size_t Parse<size_t>::operator()(Parser& p) const {
        int c = p.consume();
        if (c < '0' || c > '9') {
            throw ParseError(p, "Expected numeric character, found '{}'", static_cast<char>(c));
        }

        size_t value = static_cast<size_t>(c - '0');

        c = p.peek();
        while (c >= '0' && c <= '9') {
            value *= 10;
            value += static_cast<size_t>(c - '0');
            p.consume();
            c = p.peek();
        }

        return value;
    }

    std::string Parse<std::string>::operator()(Parser& p) const {
        p.expect('"');
        auto ss = std::stringstream();

        auto escape = [&p](int c) {
            switch (c) {
                case 'a': return '\a';
                case 'b': return '\b';
                case 'e': return '\e';
                case 'f': return '\f';
                case 'n': return '\n';
                case 'r': return '\r';
                case 't': return '\t';
                case 'v': return '\v';
                case '\\': return '\\';
                case '\'': return '\'';
                case '"': return '"';
                default:
                    throw ParseError(p, "Faulty escape character '{}'", static_cast<char>(c));
            }
        };

        int c = p.peek();
        while (c > 0 && c != '"') {
            if (c == '\\') {
                p.consume();
                ss << escape(p.peek());
            } else {
                ss << static_cast<char>(c);
            }

            p.consume();
            c = p.peek();
        }

        p.expect('"');

        return ss.str();
    }

    vk::Offset2D Parse<vk::Offset2D>::operator()(Parser& p) const {
        auto [x, y] = Parse<std::pair<size_t, size_t>>{}(p);
        return {
            static_cast<int32_t>(x),
            static_cast<int32_t>(y)
        };
    }

    vk::Extent2D Parse<vk::Extent2D>::operator()(Parser& p) const {
        auto [x, y] = Parse<std::pair<size_t, size_t>>{}(p);
        return {
            static_cast<uint32_t>(x),
            static_cast<uint32_t>(y)
        };
    }
}

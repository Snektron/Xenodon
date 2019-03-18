#include "config/Config.h"
#include <cctype>

Parser::Parser(std::istream& input):
    input(input),
    line(1),
    column(1) {
}

int Parser::peek() {
    int c = this->input.peek();

    // skip comments
    if (c == '#') {
        while (c != '\n' && c > 0) {
            c = this->input.get();
        }
    }

    return c;
}

int Parser::consume() {
    if (this->input.eof()) {
        throw ParseError(this->error("Unexpected end of file"));
    }

    int c = this->input.get();
    if (c == '\n') {
        ++this->line;
        this->column = 0;
    }

    ++this->column;        

    return c;
}

void Parser::expect(int expected) {
    int actual = this->peek();
    if (actual != expected) {
        throw ParseError(this->fmt_error([expected, actual](auto& ss) {
            ss << "Expected character '" << expected << "', found '" << actual << "'";
        }));
    }
    this->consume();
}

std::string Parser::parse_key() {
    int c = this->consume();

    if (!std::isalpha(c)) {
        throw ParseError(this->fmt_error([c](auto& ss) {
            ss << "Expected alphabetic key, found '" << c << '\'';
        }));
    }

    auto key = std::stringstream();
    key << static_cast<char>(c);
    while (std::isalpha(this->peek())) {
        key << static_cast<char>(this->consume());
    }

    return key.str();
}
#include "present/direct/DisplayConfig.h"
#include <stdexcept>
#include <sstream>
#include <cctype>

using ParseError = DisplayConfig::ParseError;

namespace {
    struct Parser {
        std::istream& input;
        size_t line, col;

        int peek();
        int consume();
        std::stringstream error();
        void expect(int c);
    };

    int Parser::peek() {
        return this->input.peek();
    }

    int Parser::consume() {
        int c = this->input.get();
        if (c == '\n') {
            ++this->line;
            this->col = 0;
        } else if (this->input.eof()) {
            auto msg = this->error();
            msg << "Unexpected end of file";
            throw ParseError(msg.str());
        }

        ++this->col;        

        return c;
    }

    std::stringstream Parser::error() {
        auto ss = std::stringstream();
        ss << "Parse error at " << this->line << ':' << this->col << ": ";
        return ss;
    }

    void Parser::expect(int c) {
        int a = this->consume();
        if (a != c) {
            auto msg = this->error();
            msg << "Unexpected character '" << static_cast<char>(a)
                << "', expected '" << static_cast<char>(c) << "'";
            throw ParseError(msg.str());
        }
    }

    void skip_whitespace(Parser& p) {
        while (isspace(p.peek())) {
            p.consume();
        }
    }

    void whitespace(Parser& p) {
        if (!isspace(p.peek())) {
            auto msg = p.error();
            msg << "Unexpected character '" << static_cast<char>(p.consume()) << "', expected whitespace";
            throw ParseError(msg.str()); 
        }

        skip_whitespace(p);        
    }

    size_t parse_int(Parser& p) {
        int c = p.consume();
        if (c < '0' || c > '9') {
            auto msg = p.error();
            msg << "Unexpected character '" << static_cast<char>(c) << "', expected integer";
            throw ParseError(msg.str());
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

    vk::Offset2D parse_offset(Parser& p) {
        p.expect('(');
        skip_whitespace(p);
        size_t x = parse_int(p);
        skip_whitespace(p);
        p.expect(',');
        skip_whitespace(p);
        size_t y = parse_int(p);
        skip_whitespace(p);
        p.expect(')');
        return vk::Offset2D{
            static_cast<int32_t>(x),
            static_cast<int32_t>(y)
        };
    }

    void match_word(Parser& p, std::string_view word) {
        for (size_t i = 0; i < word.size(); ++i) {
            int c = p.consume();
            if (c != word[i]) {
                auto msg = p.error();
                msg << "Unexpected character '" << static_cast<char>(c) << "', expected '" << word << '\'';
                throw ParseError(msg.str());
            }
        }
    }

    DisplayConfig::Screen parse_screen(Parser& p) {
        match_word(p, "screen");
        whitespace(p);
        size_t index = parse_int(p);
        whitespace(p);
        match_word(p, "at");
        whitespace(p);
        vk::Offset2D offset = parse_offset(p);
        return {
            static_cast<uint32_t>(index),
            offset
        };
    }

    template <typename T>
    bool check_unique(const std::vector<T>& items, uint32_t vulkan_index) {
        for (auto&& item : items) {
            if (item.vulkan_index == vulkan_index) {
                return false;
            }
        }

        return true;
    }

    DisplayConfig::Device parse_device(Parser& p) {
        match_word(p, "device");
        whitespace(p);
        size_t index = parse_int(p);
        whitespace(p);
        p.expect('{');
        skip_whitespace(p);
        if (p.peek() == '}') {
            p.consume();
            auto msg = p.error();
            msg << "Expected at least one display per device";
            throw ParseError(msg.str());
        }

        auto device = DisplayConfig::Device{
            static_cast<uint32_t>(index),
            {parse_screen(p)}
        };

        while (true) {
            skip_whitespace(p);
            if (p.peek() == '}')
                break;

            p.expect(',');
            skip_whitespace(p);
            auto screen = parse_screen(p);

            if (!check_unique(device.screens, screen.vulkan_index)) {
                auto msg = p.error();
                msg << "Screen index " << screen.vulkan_index << " is not unique";
                throw ParseError(msg.str());
            }

            device.screens.push_back(screen);
        }

        p.expect('}');

        return device;
    }
}

DisplayConfig::DisplayConfig(std::istream& input) {
    Parser p{input, 1};
    skip_whitespace(p);
    while (p.peek() >= 0) {
        auto gpu = parse_device(p);

        if (!check_unique(this->gpus, gpu.vulkan_index)) {
            auto msg = p.error();
            msg << "Device index " << gpu.vulkan_index << " is not unique";
            throw ParseError(msg.str());
        }

        this->gpus.push_back(gpu);
        skip_whitespace(p);
    }

    if (this->gpus.empty()) {
        auto msg = p.error();
        msg << "Expected at least one device" << std::endl;
        throw ParseError(msg.str());
    }
}

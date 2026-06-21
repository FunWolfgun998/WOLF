#pragma once

#include <string>
#include <utility>
#include <variant>
#include <cstdint>
#include "TokenType.h"

using LiteralValue = std::variant<std::monostate, long long, double, std::string, uint32_t>;
std::string tokenTypeToString(TokenType type);

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
    LiteralValue literalValue; // Variante sicura!
    int id; // for String Interning

    // Basic Constructor (std::monostate è il valore di default della variante)
    Token(TokenType type, std::string lexeme, int line, int col)
        : type(type), lexeme(std::move(lexeme)), line(line), column(col), literalValue(std::monostate{}), id(0) {}

    // Constructor for Identifiers
    Token(TokenType type, std::string lexeme, int line, int col, int id)
        : type(type), lexeme(std::move(lexeme)), line(line), column(col), literalValue(std::monostate{}), id(id) {}

    // Constructor for Literals
    Token(TokenType type, std::string lexeme, int line, int col, LiteralValue value)
        : type(type), lexeme(std::move(lexeme)), line(line), column(col), literalValue(std::move(value)), id(0) {}


    long long asInt() const {
        if (auto p = std::get_if<long long>(&literalValue)) return *p;
        return 0;
    }

    double asDouble() const {
        if (auto p = std::get_if<double>(&literalValue)) return *p;
        return 0.0;
    }

    std::string asString() const {
        if (auto p = std::get_if<std::string>(&literalValue)) return *p;
        return "";
    }

    uint32_t asChar() const {
        if (auto p = std::get_if<uint32_t>(&literalValue)) return *p;
        return '\0';
    }

    uint32_t asUnicode() const {
        if (auto p = std::get_if<uint32_t>(&literalValue)) return *p;
        return 0;
    }
    // Debug
    std::string toString() const;
};

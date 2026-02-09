#include "../../include/lexer/Token.h"

std::string Token::toString() const {
    return "Token(" + std::to_string((int)type) + ", '" + lexeme + "', riga:" + std::to_string(line) + ")";
}
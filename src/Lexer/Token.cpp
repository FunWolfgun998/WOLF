#include "../../include/Lexer/Token.h"

#include "../../include/Lexer/Token.h"

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::ERROR: return "ERROR";
        case TokenType::UNKNOWN: return "UNKNOWN";

        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::INDENT: return "INDENT";
        case TokenType::DEDENT: return "DEDENT";

        case TokenType::INT_LITERAL: return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::HEX_LITERAL: return "HEX_LITERAL";
        case TokenType::BIN_LITERAL: return "BIN_LITERAL";
        case TokenType::OCT_LITERAL: return "OCT_LITERAL";
        case TokenType::CHAR_LITERAL: return "CHAR_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::RAW_STRING_LITERAL: return "RAW_STRING";
        case TokenType::FORMAT_STRING_LITERAL: return "FORMAT_STRING";

        case TokenType::KW_INT: return "KW_INT";
        case TokenType::KW_FLOAT: return "KW_FLOAT";
        case TokenType::KW_CHAR: return "KW_CHAR";
        case TokenType::KW_STRING: return "KW_STRING";
        case TokenType::KW_VOID: return "KW_VOID";
        case TokenType::KW_BOOL: return "KW_BOOL";

        case TokenType::KW_PUBLIC: return "KW_PUBLIC";
        case TokenType::KW_PRIVATE: return "KW_PRIVATE";
        case TokenType::KW_PROTECTED: return "KW_PROTECTED";
        case TokenType::KW_THIS: return "KW_THIS";
        case TokenType::KW_EXTENDS: return "KW_EXTENDS";

        case TokenType::KW_IF: return "KW_IF";
        case TokenType::KW_ELSE: return "KW_ELSE";
        case TokenType::KW_ELIF: return "KW_ELIF";
        case TokenType::KW_WHILE: return "KW_WHILE";
        case TokenType::KW_FOR: return "KW_FOR";
        case TokenType::KW_IN: return "KW_IN";
        case TokenType::KW_RETURN: return "KW_RETURN";
        case TokenType::KW_BREAK: return "KW_BREAK";
        case TokenType::KW_CONTINUE: return "KW_CONTINUE";
        case TokenType::KW_STRUCT: return "KW_STRUCT";
        case TokenType::KW_CLASS: return "KW_CLASS";
        case TokenType::KW_NULL: return "KW_NULL";
        case TokenType::KW_TRUE: return "KW_TRUE";
        case TokenType::KW_FALSE: return "KW_FALSE";

        case TokenType::OP_SHIFT_LEFT: return "OP_SHIFT_LEFT";
        case TokenType::OP_SHIFT_RIGHT: return "OP_SHIFT_RIGHT";
        case TokenType::OP_SHIFT_LEFT_ASS: return "OP_SHIFT_LEFT_ASS";
        case TokenType::OP_SHIFT_RIGHT_ASS: return "OP_SHIFT_RIGHT_ASS";
        case TokenType::OP_EQ_EQ: return "OP_EQ_EQ";
        case TokenType::OP_NOT_EQ: return "OP_NOT_EQ";
        case TokenType::OP_LESS_EQ: return "OP_LESS_EQ";
        case TokenType::OP_GRT_EQ: return "OP_GRT_EQ";
        case TokenType::OP_AND: return "OP_AND";
        case TokenType::OP_OR: return "OP_OR";
        case TokenType::OP_PLUS_ASS: return "OP_PLUS_ASS";
        case TokenType::OP_MIN_ASS: return "OP_MIN_ASS";
        case TokenType::OP_MUL_ASS: return "OP_MUL_ASS";
        case TokenType::OP_DIV_ASS: return "OP_DIV_ASS";
        case TokenType::OP_MOD_ASS: return "OP_MOD_ASS";
        case TokenType::OP_AND_ASS: return "OP_AND_ASS";
        case TokenType::OP_OR_ASS: return "OP_OR_ASS";
        case TokenType::OP_XOR_ASS: return "OP_XOR_ASS";
        case TokenType::RANGE_OP: return "RANGE_OP";
        case TokenType::ARROW: return "ARROW";

        case TokenType::OP_BIT_AND: return "OP_BIT_AND";
        case TokenType::OP_BIT_OR: return "OP_BIT_OR";
        case TokenType::OP_PLUS: return "OP_PLUS";
        case TokenType::OP_MINUS: return "OP_MINUS";
        case TokenType::OP_STAR: return "OP_STAR";
        case TokenType::OP_SLASH: return "OP_SLASH";
        case TokenType::OP_MOD: return "OP_MOD";
        case TokenType::OP_XOR: return "OP_XOR";
        case TokenType::OP_ASSIGN: return "OP_ASSIGN";
        case TokenType::OP_NOT: return "OP_NOT";
        case TokenType::OP_LESS: return "OP_LESS";
        case TokenType::OP_GRT: return "OP_GRT";
        case TokenType::OP_TILDE: return "OP_TILDE";
        case TokenType::OP_QUESTION: return "OP_QUESTION";
        case TokenType::OP_INC: return "INCREMENT";
        case TokenType::OP_DEC: return "DECREMENT";

        case TokenType::DOT: return "DOT";
        case TokenType::COLON: return "COLON";
        case TokenType::L_PAREN: return "L_PAREN";
        case TokenType::R_PAREN: return "R_PAREN";
        case TokenType::L_BRACKET: return "L_BRACKET";
        case TokenType::R_BRACKET: return "R_BRACKET";
        case TokenType::COMMA: return "COMMA";

        case TokenType::IDENTIFIER: return "IDENTIFIER";

        default: return "UNKNOWN";
    }
}

std::string Token::toString() const {
    // Pulizia per la stampa
    std::string cleanLexeme = lexeme;
    if (cleanLexeme == "\n") cleanLexeme = "\\n";

    std::string out = "Type: " + tokenTypeToString(type) +
                      ", Value: '" + cleanLexeme + "'" +
                      ", Line: " + std::to_string(line) +
                      ", Column: " + std::to_string(column);

    // Stampa ID se è identificatore (Ora funzionerà!)
    if (id != 0) {
        out += ", ID: " + std::to_string(id);
    }
    // Stampa il valore vero convertito (La magia di std::variant)
    if (std::holds_alternative<long long>(literalValue)) {
        out += ", IntVal: " + std::to_string(std::get<long long>(literalValue));
    } else if (std::holds_alternative<double>(literalValue)) {
        out += ", FloatVal: " + std::to_string(std::get<double>(literalValue));
    } else if (std::holds_alternative<std::string>(literalValue)) {
        out += ", StrVal: \"" + std::get<std::string>(literalValue) + "\"";
    } else if (std::holds_alternative<char>(literalValue)) {
        out += std::string(", CharVal: '") + std::get<char>(literalValue) + "'";
    }
    return out;
}
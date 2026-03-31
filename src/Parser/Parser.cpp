#include "../../include/Parser/Parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

std::vector<Stmt> Parser::parse() {
    std::vector<Stmt> statements;
    while (!isAtEnd()) {
        // Qui chiameremo parseStatement() quando lo definiremo
        // statements.push_back(parseStatement());
    }
    return statements;
}

// Navigation Helpers
Token Parser::peek() const {
    return tokens[current];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return tokens[current - 1]; // Restituisce quello appena "superato"
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();

    // Trigger Error + Panic Mode
    error(peek(), message);
    throw ParseError();
}

// --- Error Handling ---

void Parser::error(const Token& token, const std::string& message) {
    std::cerr << "[Parse Error] Line " << token.line
              << ", Col " << token.column << ": " << message;

    if (token.type == TokenType::END_OF_FILE) {
        std::cerr << " at end of file.";
    } else {
        std::cerr << " at '" << token.lexeme << "'";
    }
    std::cerr << std::endl;
}

void Parser::synchronize() {
    advance(); // Consume the offending token

    while (!isAtEnd()) {
        // If we see a newline, we are likely at the start of a new statement
        if (peek().type == TokenType::NEWLINE) return;

        // If we see a structure keyword, restart parsing from there
        switch (peek().type) {
            case TokenType::KW_CLASS:
            case TokenType::KW_STRUCT:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_FOR:
            case TokenType::KW_RETURN:
            case TokenType::INDENT:
            case TokenType::DEDENT:
                return;
            default:
                break;
        }
        advance();
    }
}
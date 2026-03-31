#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include "../Lexer/Token.h"
#include "../AST/ASTNodes.h"

// Exception used to trigger Panic Mode error recovery
class ParseError : public std::runtime_error {
public:
    ParseError() : std::runtime_error("") {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::vector<Stmt> parse(); // Entry point

private:
    const std::vector<Token>& tokens;
    size_t current = 0; // The index of the token we are currently looking at

    Token peek() const;   // Look at the current token
    Token advance();      // Consume current and return it
    bool isAtEnd() const; // Check if we hit EOF
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& message);

    // Error Handling
    void error(const Token& token, const std::string& message);
    void synchronize();   // Panic mode: skip tokens until a safe point
};
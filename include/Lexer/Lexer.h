#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <charconv>
#include <stack>
#include "Token.h"

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    std::string source;
    std::vector<Token> tokens;

    // Pointers for scanning
    size_t start = 0;       // Start of the current token being scanned
    size_t current = 0;     // Current character being inspected

    // Source location tracking
    int line = 1;
    int column = 1;
    int startColumn = 1;    // Column where the current token starts

    // State for Python-style indentation
    std::vector<int> indentStack;

    // String Interning pool for identifiers
    std::unordered_map<std::string, int> identifierIDs;
    int nextID = 1;

    // --- Primitive Operations ---
    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext(int n = 1) const;
    bool match(char expected);

    // --- Core Logic ---
    void scanToken();
    void string();
    void rawString();
    void formatString();
    void charLiteral();
    void number();
    void identifier();
    void handleIndentation();

    // --- Error Handling ---
    void manageError(const std::string& message);
    void recoverFromError();

    // --- Token Emission Helpers ---
    void addToken(TokenType type);
    void addIdentifierToken(TokenType type, int id);
    void addToken(TokenType type, LiteralValue value);
};

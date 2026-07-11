#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include "../Lexer/Token.h"
#include "../Lexer/Lexer.h"
#include "../AST/ASTNodes.h"

/*
    Binding Power (Precedence) definitions for the Pratt Parser.
    Higher values bind tighter (e.g. * binds tighter than +).
*/
enum class Precedence {
    NONE = 0,
    ASSIGNMENT,  // = += -=
    OR,          // ||
    AND,         // &&
    EQUALITY,    // == !=
    COMPARISON,  // < > <= >=
    TERM,        // + -
    FACTOR,      // * / %
    UNARY,       // ! - (Prefix operations)
    CALL,        // . () []
    PRIMARY      // Highest binding power (Numbers, Variables, Grouping)
};

// Exception used to trigger Panic Mode error recovery
class ParseError : public std::runtime_error {
public:
    ParseError() : std::runtime_error("") {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::vector<Stmt> parse();

private:
    const std::vector<Token>& tokens;
    size_t current = 0;

    // --- 1. STATEMENT PARSERS (Recursive Descent) ---
    Stmt parseStatement();
    BlockStmt parseBlock(); // Handle Indent and Dedent
    Stmt parseVarDecl();
    Stmt parseIfStmt();
    Stmt parseWhileStmt();
    Stmt parseForStmt();
    Stmt parseFunctionDecl();
    Stmt parseStructDecl();
    Stmt parseClassDecl();
    Stmt parseReturnStmt();
    Stmt parseBreakStmt();
    Stmt parseContinueStmt();

    // --- 2. EXPRESSION PARSERS (Node Builders) ---
    Expr parseExpression();
    // Parses an expression stopping when it hits an operator with lower precedence
    Expr parsePrecedence(Precedence precedence);

    Expr parseLiteral(Token token);
    Expr parseFormatString(Token token);
    Expr parseVariable(Token token);
    Expr parseGrouping(Token openingParen);
    Expr parseArrayLiteral(Token openingBracket);
    Expr parseUnary(Token op);
    Expr parsePostfix(Expr left, Token op);
    Expr parseBinary(Expr left, Token op);
    Expr parseRange(Expr left, Token opToken);
    Expr parseCall(Expr callee, Token openingParen);
    Expr parseArrayAccess(Expr indexed, Token openingBracket);
    Expr parseMemberAccess(Expr accessed, Token dot);
    Expr parseTernary(Expr condition, Token questionMark);


    // --- 3. PRATT PARSER HELPERS (Lookup Tables) ---
    /*
        A rule for tokens that don't need a left-hand side.
        Example: Numbers (5), Unary minus (-5), Opening parenthesis ( (1+2) )
    */
    using NudHandlerMethod = Expr (Parser::*)(Token token);

    /*
        A rule for tokens that glue two expressions together.
        It requires the already parsed left-hand side expression.
        Example: Binary plus (+), Function call ( () )
    */
    using LedHandlerMethod = Expr (Parser::*)(Expr left, Token opToken);

    Precedence getPrecedence(TokenType type) const;
    NudHandlerMethod getNudHandler(TokenType type) const;
    LedHandlerMethod getLedHandler(TokenType type) const;

    // --- 4. NAVIGATION HELPERS ---
    Token advance();
    Token peek() const;
    Token peeknNext(int n) const;
    Token consume(TokenType type, const std::string& message);
    bool match(TokenType type);
    bool check(TokenType type) const;
    bool isAtEnd() const;
    bool isTypeToken(TokenType type) const;

    // --- 5. ERROR HANDLING ---
    void error(const Token& token, const std::string& message);
    void synchronize();
};
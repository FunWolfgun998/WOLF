#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include "../Lexer/Token.h"
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

    // --- Navigation Helpers ---
    //Rispettando alla lettera la regola: non si guarda mai indietro cattureremo il token mentre lo consumiamo con advance()..
    Token peek() const;
    Token peeknNext(int n) const;

    Token advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& message);

    // --- Error Handling ---
    void error(const Token& token, const std::string& message);
    void synchronize();

    //Statement parser methods (Recursive Descent)
    Stmt parseStatement();
    Stmt parseVarDecl();
    Stmt parseIfStmt();
    Stmt parseWhileStmt();
    Stmt parseForStmt();

    Stmt parseFunctionDecl(Token returnType);

    Stmt parseFunctionDecl();
    Stmt parseStructDecl();
    Stmt parseReturnStmt();
    BlockStmt parseBlock(); //Handel Indent and Dedent


    // Pratt parser methods (Expressions)

    // Parses an expression stopping when it hits an operator with lower precedence
    Expr parsePrecedence(Precedence precedence);
    Expr parsePostfix(Expr left, Token opToken);
    Expr parseTernary(Expr left, Token opToken);
    Expr parseExpression();
    // expression methods (Node Builders)

    Expr parseLiteral(Token token);
    Expr parseVariable(Token token);
    Expr parseGrouping(Token token);
    Expr parseUnary(Token token);
    Expr parseBinary(Expr left, Token opToken);
    Expr parseCall(Expr left, Token parenToken);
    Expr parseMemberAccess(Expr left, Token dotToken);
    Expr parseArrayAccess(Expr left, Token bracketToken);

    // --- LOOKUP TABLES (Function Pointers) ---

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

    // Helper methods to fetch the correct rule and precedence for a given token type
    NudHandlerMethod getNudHandler(TokenType type) const;
    LedHandlerMethod getLedHandler(TokenType type) const;
    Precedence getPrecedence(TokenType type) const;
};
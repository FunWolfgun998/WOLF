#pragma once

#include "ASTdec.h"
#include "../Lexer/Token.h"

// --- EXPRESSION NODES ---

struct LiteralExpr {
    Token value; // Holds the literal value (int, float, string, etc.)
};

struct VariableExpr {
    Token name; // Identifier name and ID
};

struct BinaryExpr {
    Expr left;   // Left expression
    Token op;    // Operator token (+, -, *, ==)
    Expr right;  // Right expression
};

struct UnaryExpr {
    Token op;    // Operator (-, !, ~, ++, --)
    Expr right;  // The operand expression
};

struct GroupingExpr {
    Expr expression; // Expression inside parentheses: ( 1 + 2 )
};

// --- STATEMENT NODES ---

struct ExpressionStmt {
    Expr expression; // An expression used as a standalone statement
};

struct VarDeclStmt {
    Token type;         // Variable type (e.g., KW_INT)
    Token name;         // Variable identifier
    std::unique_ptr<Expr> initializer; // Optional initial value
};

struct BlockStmt {
    std::vector<Stmt> statements; // List of statements inside a block
};

struct IfStmt {
    Expr condition;               // Condition to evaluate
    BlockStmt thenBranch;         // Code to execute if condition is true
    std::unique_ptr<BlockStmt> elseBranch; // Optional 'else' or 'elif' block
};

struct WhileStmt {
    Expr condition;
    BlockStmt body;
};

struct ReturnStmt {
    Token keyword;               // The 'return' token
    std::unique_ptr<Expr> value; // Optional return value
};

// --- AST FACTORY HELPERS ---

template<typename T, typename... Args>
Expr makeExpr(Args&&... args) {
    return Expr{ std::make_unique<T>(std::forward<Args>(args)...) };
}

template<typename T, typename... Args>
Stmt makeStmt(Args&&... args) {
    return Stmt{ std::make_unique<T>(std::forward<Args>(args)...) };
}
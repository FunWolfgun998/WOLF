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

// Expression inside parentheses: ( 1 + 2 )
struct GroupingExpr {
    Expr expression;
};

// --- STATEMENT NODES ---

// An expression used as a standalone statement
struct ExpressionStmt {
    Expr expression;
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
    Expr condition;
    Stmt thenBranch;
    std::vector<ElseIfBranch> elifBranches; // vector of elif
    std::unique_ptr<Stmt> elseBranch; // Possible final else
};

struct ElseIfBranch {
    Expr condition;
    Stmt block;
};

struct WhileStmt {
    Expr condition;
    Stmt body;
};
// return "Hello XD"
struct ReturnStmt {
    std::unique_ptr<Expr> value; // Optional return value
};

// functionCall(arg1, arg2)
struct CallExpr {
    Expr callee; // L'espressione da chiamare (spesso un VariableExpr)
    Token paren; // Il token '(' per il debug/errori
    std::vector<Expr> arguments;
};

// object.field
struct MemberAccessExpr {
    Expr object;
    Token name; // Il nome del campo o metodo
};

// array[index]
struct ArrayAccessExpr {
    Expr array;
    Expr index;
};

// for x in start..end:
struct ForStmt {
    Token iteratorVar; // variable 'x'
    Expr startRange;   // start (es. 0)
    Expr endRange;     // end (es. 10)
    Stmt body;
};

// int myFunc(int a, float b):
struct Parameter {
    Token type;
    Token name;
};

struct TernaryExpr {
    Expr condition;
    Expr trueBranch;
    Expr falseBranch;
};

struct FunctionDeclStmt {
    Token returnType;
    Token name;
    std::vector<Parameter> parameters;
    Stmt body;
};

// struct Point:
struct StructDeclStmt {
    Token name;
    std::vector<Stmt> fields;
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
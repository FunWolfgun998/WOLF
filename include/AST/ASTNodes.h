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
    Expr condition;
    BlockStmt thenBranch;
    std::vector<ElseIfBranch> elifBranches; // vector of elif
    std::unique_ptr<BlockStmt> elseBranch;  // Possible final else
};

struct ElseIfBranch {
    Expr condition;
    BlockStmt block;
};

struct WhileStmt {
    Expr condition;
    BlockStmt body;
};
// return "Hello XD"
struct ReturnStmt {
    Token keyword;               // The 'return' token
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
    BlockStmt body;
};

// int myFunc(int a, float b):
struct Parameter {
    Token type;
    Token name;
};

struct FunctionDeclStmt {
    Token returnType;
    Token name;
    std::vector<Parameter> parameters;
    BlockStmt body;
};

// struct Point:
struct StructDeclStmt {
    Token name;
    std::vector<VarDeclStmt> fields; // I campi della struct (trattati come dichiarazioni di variabili)
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
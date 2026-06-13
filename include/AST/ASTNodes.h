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

// 5 + var
struct BinaryExpr {
    Expr left;   // Left expression
    Token op;    // Operator token (+, -, *, ==)
    Expr right;  // Right expression
};

// 1..10
struct RangeExpr {
    Expr start;
    Token op;
    Expr end;
};

// i++ or ++j
struct UnaryExpr {
    Token op;       // Operator (-, !, ~, ++, --)
    Expr operand;     // The operand expression
    bool isPostfix; // Needed to differentiate ++x from x++
};

// Expression inside parentheses: ( 1 + 2 )
struct GroupingExpr {
    Token openingParen; // The '(' token for debug/error tracking
    Expr expression;
};

// --- STATEMENT NODES ---

// An expression used as a standalone statement
struct ExpressionStmt {
    Expr expression;
};
// int i or int j = 0 or Person programmer = Person("Cristian")
struct VarDeclStmt {
    Token type;                        // Variable type (e.g. KW_INT)
    Token name;                        // Variable identifier
    std::unique_ptr<Expr> initializer; // Optional initial value
};

struct BlockStmt {
    std::vector<Stmt> statements; // List of statements inside a block
};

//  if x == 1:
//      ...
//  elif x != 998:
//      ...
//  else:
//      ...
struct IfStmt {
    Token keyword;                          // The 'if' token
    Expr condition;
    Stmt thenBranch;                        // Body of the 'if'
    std::vector<ElseIfBranch> elifBranches; // Vector of elif branches
    std::unique_ptr<Stmt> elseBranch;       // Possible final 'else' branch (optional)
};

// elif x !=5:
//      x = 9
//      ...
struct ElseIfBranch {
    Token keyword; // The 'elif' token
    Expr condition;
    Stmt block;    // Body of the 'elif'
};

//  while(i<=998):
//      i++
//      ...
struct WhileStmt {
    Token keyword;  // The 'while' token
    Expr condition;
    Stmt body;      // Body of the loop
};

// return or return "Hello XD"
struct ReturnStmt {
    Token keyword;               // The 'return' token
    std::unique_ptr<Expr> value; // Optional return value
};

//  struct Point:
//      int x
//      ...
struct StructDeclStmt {
    Token name;
    std::vector<Stmt> fields; // Fields of the struct (must be VarDeclStmt)
};

// functionCall(arg1, arg2)
struct CallExpr {
    Expr callee;                 // Expression being called (often a VariableExpr)
    Token paren;                 // The '(' token for debug/error tracking
    std::vector<Expr> arguments; // List of arguments passed
};

// accessed.member
struct MemberAccessExpr {
    Expr accessed;
    Token dot;     // The '.' token for error tracking
    Token member;  // The identifier (field or method name)
};

// array[index]
struct ArrayAccessExpr {
    Expr array;
    Token openingBracket; // The '[' token for error tracking
    Expr index;
};

// [1, 2, 4]
struct ArrayLiteralExpr {
    Token openingBracket;      // The '[' token
    std::vector<Expr> elements; // Elements of the array
};

// for x in start..end:
struct ForStmt {
    Token keyword;     // CORRETTO: era 'keywork'
    Token iteratorVar; // Variable 'x'
    Expr startRange;   // Start of range (e.g. 0)
    Expr endRange;     // End of range (e.g. 10)
    Stmt body;         // Body of the loop
};

// int myFunc(int a, float b):
struct Parameter {
    Token type;
    Token name;
    std::unique_ptr<Expr> defaultValue; // Optional default value (e.g. int a = 998)
};

struct TernaryExpr {
    Expr condition;
    Token questionMark; // The '?' token for error tracking
    Expr trueBranch;
    Expr falseBranch;
};

struct FunctionDeclStmt {
    Token returnType;
    Token name;
    std::vector<Parameter> parameters;
    Stmt body; // Body of the function
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
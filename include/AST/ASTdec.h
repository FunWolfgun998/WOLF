#pragma once

#include <memory>
#include <variant>
#include <vector>

// Forward declarations of AST nodes to resolve circular dependencies
struct LiteralExpr;
struct VariableExpr;
struct BinaryExpr;
struct UnaryExpr;
struct GroupingExpr;

struct ExpressionStmt;
struct VarDeclStmt;
struct BlockStmt;
struct IfStmt;
struct WhileStmt;
struct ReturnStmt;

// Variant containing all possible expression types
using ExprVariant = std::variant<
    std::unique_ptr<LiteralExpr>,
    std::unique_ptr<VariableExpr>,
    std::unique_ptr<BinaryExpr>,
    std::unique_ptr<UnaryExpr>,
    std::unique_ptr<GroupingExpr>
>;

// Wrapper struct for an expression
struct Expr {
    ExprVariant as; 
};

// Variant containing all possible statement types
using StmtVariant = std::variant<
    std::unique_ptr<ExpressionStmt>,
    std::unique_ptr<VarDeclStmt>,
    std::unique_ptr<BlockStmt>,
    std::unique_ptr<IfStmt>,
    std::unique_ptr<WhileStmt>,
    std::unique_ptr<ReturnStmt>
>;

// Wrapper struct for a statement
struct Stmt {
    StmtVariant as;
};
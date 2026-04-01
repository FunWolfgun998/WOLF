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
struct ElseIfBranch;
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

// Wrapper struct for an expression.
/*
    Is a Sum Type of all possible type of node.
    Is like
    class ExprNode
    {
    }
    class BinaryNode : public BaseNode{

    }
    Instead of classes we use structs and at the same time allows
    us to have some nodes to enter different groups of nodes (other than Expr, like Stmt)
 */

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
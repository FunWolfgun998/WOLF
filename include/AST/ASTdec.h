#pragma once

#include <memory>
#include <variant>
#include <optional>

// Forward declarations of AST nodes to resolve circular dependencies
struct LiteralExpr;
struct VariableExpr;
struct BinaryExpr;
struct RangeExpr;
struct UnaryExpr;
struct GroupingExpr;
struct CallExpr;
struct MemberAccessExpr;
struct ArrayAccessExpr;
struct ArrayLiteralExpr;

struct ExpressionStmt;
struct VarDeclStmt;
struct BlockStmt;
struct IfStmt;
struct ElseIfBranch;
struct WhileStmt;
struct ForStmt;
struct FunctionDeclStmt;
struct StructDeclStmt;
struct ClassDeclStmt;
struct ThisExpr;
struct ReturnStmt;
struct BreakStmt;
struct ContinueStmt;
struct TernaryExpr;


// Variant containing all possible expression types
using ExprVariant = std::variant<
    std::unique_ptr<LiteralExpr>,
    std::unique_ptr<VariableExpr>,
    std::unique_ptr<BinaryExpr>,
    std::unique_ptr<RangeExpr>,
    std::unique_ptr<UnaryExpr>,
    std::unique_ptr<GroupingExpr>,
    std::unique_ptr<CallExpr>,
    std::unique_ptr<MemberAccessExpr>,
    std::unique_ptr<ArrayAccessExpr>,
    std::unique_ptr<ArrayLiteralExpr>,
    std::unique_ptr<TernaryExpr>,
    std::unique_ptr<ThisExpr>
>;

// Wrapper struct for an expression.
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
    std::unique_ptr<ForStmt>,
    std::unique_ptr<FunctionDeclStmt>,
    std::unique_ptr<StructDeclStmt>,
    std::unique_ptr<ClassDeclStmt>,
    std::unique_ptr<ReturnStmt>,
    std::unique_ptr<BreakStmt>,
    std::unique_ptr<ContinueStmt>
>;

// Wrapper struct for a statement
struct Stmt {
    StmtVariant as;
};
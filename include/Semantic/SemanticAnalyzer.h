#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "../AST/ASTdec.h"
#include "../AST/ASTNodes.h"
#include "Type.h"
#include "SymbolTable.h"

class SemanticAnalyzer {
private:
    // Tracks active lexical scopes and resolved symbols
    SymbolTable symbolTable;

    // Collects all diagnostics without halting compilation on the first error
    std::vector<std::string> errors;

    // Active context pointers for contextual validation
    const Type* currentClass = nullptr;
    const Type* currentFunctionReturnType = nullptr;
    int loopDepth = 0;

    // Memory pool owning user-defined types (classes, structs) created during analysis
    std::vector<std::unique_ptr<Type>> customTypes;

    // Logs a formatted semantic error with source token coordinates
    void reportError(const Token& token, const std::string& message);

    // Maps a type token (e.g. "int", "string[]", "Dog") to its corresponding Type* pointer
    const Type* resolveTypeFromToken(const Token& typeToken);

    // Verifies whether an expression represents an assignable memory location (variable, field, array index)
    bool isLValue(const Expr& expr) const;

public:
    SemanticAnalyzer();

    // Entry point: orchestrates Pass 1 (declarations) and Pass 2 (type checking)
    bool analyze(const std::vector<Stmt>& ast);

    // Accessors for collected errors
    const std::vector<std::string>& getErrors() const { return errors; }
    bool hasErrors() const { return !errors.empty(); }

    // Pass 1: Scans top-level signatures to support forward references
    void collectDeclarations(const std::vector<Stmt>& ast);

    // Pass 2: Recursively type-checks all statement bodies and sub-expressions
    void typeCheck(const std::vector<Stmt>& ast);

    // Dynamic dispatchers invoking the visitor overloads via std::visit
    const Type* evaluate(const Expr& expr);
    void execute(const Stmt& stmt);

    // Fallback handler returning ErrorType for unhandled node variants
    template<typename T>
    const Type* operator()(const T& node) {
        return Type::getError();
    }

    // Expression visitors: each evaluates and returns its resulting Type*
    const Type* operator()(const std::unique_ptr<LiteralExpr>& node);
    const Type* operator()(const std::unique_ptr<VariableExpr>& node);
    const Type* operator()(const std::unique_ptr<ThisExpr>& node);
    const Type* operator()(const std::unique_ptr<BinaryExpr>& node);
    const Type* operator()(const std::unique_ptr<RangeExpr>& node);
    const Type* operator()(const std::unique_ptr<UnaryExpr>& node);
    const Type* operator()(const std::unique_ptr<GroupingExpr>& node);
    const Type* operator()(const std::unique_ptr<CallExpr>& node);
    const Type* operator()(const std::unique_ptr<MemberAccessExpr>& node);
    const Type* operator()(const std::unique_ptr<ArrayAccessExpr>& node);
    const Type* operator()(const std::unique_ptr<ArrayLiteralExpr>& node);
    const Type* operator()(const std::unique_ptr<TernaryExpr>& node);

    // Statement visitors: execute side-effects (scoping, definitions) and return void
    void operator()(const std::unique_ptr<ExpressionStmt>& node);
    void operator()(const std::unique_ptr<VarDeclStmt>& node);
    void operator()(const std::unique_ptr<BlockStmt>& node);
    void operator()(const std::unique_ptr<IfStmt>& node);
    void operator()(const std::unique_ptr<WhileStmt>& node);
    void operator()(const std::unique_ptr<ForStmt>& node);
    void operator()(const std::unique_ptr<FunctionDeclStmt>& node);
    void operator()(const std::unique_ptr<StructDeclStmt>& node);
    void operator()(const std::unique_ptr<ClassDeclStmt>& node);
    void operator()(const std::unique_ptr<ReturnStmt>& node);
    void operator()(const std::unique_ptr<BreakStmt>& node);
    void operator()(const std::unique_ptr<ContinueStmt>& node);
};
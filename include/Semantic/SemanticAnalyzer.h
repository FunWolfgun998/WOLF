#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "../AST/ASTdec.h"
#include "../AST/ASTNodes.h"
#include "Type.h"
#include "SymbolTable.h"
#include "DiagnosticEngine.h"
#include "ErrorCode.h"

class SemanticAnalyzer {
private:
    // Tracks active lexical scopes and resolved symbols
    SymbolTable symbolTable;

    // Collects diagnostics with source coordinates and formatted output
    DiagnosticEngine diagnostics;

    // Active context pointers for contextual validation
    const Type* currentClass = nullptr;
    const Type* currentFunctionReturnType = nullptr;
    int loopDepth = 0;

    // Memory pool owning user-defined types (classes, structs) created during analysis
    std::vector<std::unique_ptr<Type>> customTypes;

    // Reports a categorized semantic error with optional fix suggestion
    void reportError(ErrorCode code, const Token& token, const std::string& message, const std::string& hint = "");

    // Gathers all currently visible identifiers across scope stack for typo suggestions
    std::vector<std::string> getVisibleSymbolNames() const;

    // Maps a type token to its corresponding Type* pointer
    const Type* resolveTypeFromToken(const Token& typeToken);

    // Verifies whether an expression represents an assignable memory location
    static bool isLValue(const Expr& expr);

    // Extracts the primary anchor token of an expression for error highlighting
    static Token getExprToken(const Expr& expr);

public:
    SemanticAnalyzer() = default;

    // Configures source file text for on-demand snippet rendering
    void setSource(std::string filename, std::string source) {
        diagnostics.setSource(std::move(filename), std::move(source));
    }

    // Orchestrates Pass 1 and Pass 2
    bool analyze(const std::vector<Stmt>& ast);

    // Diagnostic accessors
    const DiagnosticEngine& getDiagnostics() const { return diagnostics; }
    bool hasErrors() const { return diagnostics.hasErrors(); }

    // Pass 1: Registers top-level types, member layouts, and function signatures
    void collectDeclarations(const std::vector<Stmt>& ast);

    // Pass 2: Type-checks all statement bodies and expressions
    void typeCheck(const std::vector<Stmt>& ast);

    // Dispatchers using std::visit
    const Type* evaluate(const Expr& expr);
    void execute(const Stmt& stmt);

    // Fallback visitor returning ErrorType for unhandled node variants
    template<typename T>
    const Type* operator()(const T& node) {
        return Type::getError();
    }

    // Expression visitors (infer and return Type*)
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

    // Statement visitors (execute side-effects and return void)
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
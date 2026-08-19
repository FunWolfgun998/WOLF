//
// Created by funwolfgun on 6/8/26.
//
#pragma once
#include <iosfwd>
#include <string>
#include <vector>
#include  <variant>
#include <ostream>
#include <iostream>
#include <memory>
#include "../include/AST/ASTdec.h"
#include "../include/AST/ASTNodes.h"



class ASTPrinter {
private:
    std::ostream& out;
    std::string currentPrefix = "";

    template<typename Type>
    //
    void printBranch(const Type& branchTypeVariant, bool isLastBranch, const std::string& labelBranch = "") {
        out << "\n";
        out << currentPrefix;
        out << (isLastBranch ? "└── " : "├── ");
        if (!labelBranch.empty()) out << labelBranch <<": ";
        std::string oldPrefix = currentPrefix;
        currentPrefix += (isLastBranch ? "    " : "│   ");

        std::visit(*this, branchTypeVariant);
        currentPrefix = oldPrefix;
    }
public:
    //Constructor
    explicit ASTPrinter(std::ostream& outputStream = std::cout)
        : out(outputStream) {}
    // Main method called by the main function
    void printAST(const std::vector<Stmt>& statements) {
        out << "--- WOLF ABSTRACT SYNTAX TREE ---\n";
        for (size_t i = 0; i < statements.size(); ++i) {
            bool isLast = (i == statements.size() - 1);
            printBranch(statements[i].as, isLast, "Statement");
        }
    }
    //Fallback method if type is not present
    template<typename T>
    void operator()(const T& node) {
        out << "[ERROR: Printer not implemented for this node type]\n";
    }

    // -- Expressions (from ExprVariant) --
    void operator()(const std::unique_ptr<LiteralExpr>& node);
    void operator()(const std::unique_ptr<VariableExpr>& node);
    void operator()(const std::unique_ptr<BinaryExpr>& node);
    void operator()(const std::unique_ptr<UnaryExpr>& node);
    void operator()(const std::unique_ptr<GroupingExpr>& node);
    void operator()(const std::unique_ptr<CallExpr>& node);
    void operator()(const std::unique_ptr<MemberAccessExpr>& node);
    void operator()(const std::unique_ptr<ArrayAccessExpr>& node);
    void operator()(const std::unique_ptr<ArrayLiteralExpr>& node);
    void operator()(const std::unique_ptr<TernaryExpr>& node);
    void operator()(const std::unique_ptr<RangeExpr>& node);

    // -- Statements (from StmtVariant) --
    void operator()(const std::unique_ptr<ExpressionStmt>& node);
    void operator()(const std::unique_ptr<VarDeclStmt>& node);
    void operator()(const std::unique_ptr<BlockStmt>& node);
    void operator()(const std::unique_ptr<IfStmt>& node);
    void operator()(const std::unique_ptr<WhileStmt>& node);
    void operator()(const std::unique_ptr<ReturnStmt>& node);
    void operator()(const std::unique_ptr<BreakStmt>& node);
    void operator()(const std::unique_ptr<ContinueStmt>& node);
    void operator()(const std::unique_ptr<ForStmt>& node);
    void operator()(const std::unique_ptr<FunctionDeclStmt>& node);
    void operator()(const std::unique_ptr<StructDeclStmt>& node);
    void operator()(const std::unique_ptr<ThisExpr>& node);
    void operator()(const std::unique_ptr<ClassDeclStmt>& node);
};


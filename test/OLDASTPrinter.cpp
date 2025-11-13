//
// Created by Personal on 13/06/2025.
//
#include "../../test/ASTPrinter.h"
#include "AST/Nodes/AllNodes.h"
#include <iomanip>

ASTPrinter::ASTPrinter(std::ostream& out, bool showTypes)
    : out(out), showTypes(showTypes) {}

void ASTPrinter::printIndent() {
    for (int i = 0; i < indentLevel; ++i) {
        out << "  ";
    }
}

void ASTPrinter::printWithType(const std::string& value, const std::string& type) {
    if (showTypes && !type.empty()) {
        out << value << " : " << type;
    } else {
        out << value;
    }
}

void ASTPrinter::Print(const std::string& nodeType, const std::vector<ASTNodePtr>& children) {
    printIndent();
    out << nodeType << ":\n";
    indentLevel++;
    for (const auto& child : children) {
        if (child) {
            child->accept(*this);
        }
    }
    indentLevel--;
}

void ASTPrinter::Print(const std::string& nodeType, const std::string& value, const std::string& type) {
    printIndent();
    out << nodeType << ": ";
    printWithType(value, type);
    out << "\n";
}

// Implementations for each node type
void ASTPrinter::visit(const ProgramNode& node) {
    Print("Program", node.getStatements());
}

void ASTPrinter::visit(const IntLiteralNode& node) {
    Print("IntLiteral", std::to_string(node.getValue()), "int");
}

void ASTPrinter::visit(const FloatLiteralNode& node) {
    Print("FloatLiteral", std::to_string(node.getValue()), "float");
}

void ASTPrinter::visit(const StringLiteralNode& node) {
    Print("StringLiteral", "\"" + node.getValue() + "\"", "string");
}

void ASTPrinter::visit(const CharLiteralNode& node) {
    Print("CharLiteral", std::string("'") + node.getValue() + "'", "char");
}

void ASTPrinter::visit(const BoolLiteralNode& node) {
    Print("BoolLiteral", node.getValue() ? "true" : "false", "bool");
}

void ASTPrinter::visit(const NullLiteralNode& node) {
    Print("NullLiteral", "null");
}

void ASTPrinter::visit(const IdentifierNode& node) {
    Print("Identifier", node.getValue());
}

void ASTPrinter::visit(const BinaryExprNode& node) {
    printIndent();
    out << "BinaryExpr: " << node.getOp() << "\n";
    indentLevel++;
    node.getLeft()->accept(*this);
    node.getRight()->accept(*this);
    indentLevel--;
}

void ASTPrinter::visit(const UnaryExprNode& node) {
    printIndent();
    out << "UnaryExpr: " << node.getOp() << "\n";
    indentLevel++;
    node.getOperand()->accept(*this);
    indentLevel--;
}

void ASTPrinter::visit(const AssignmentNode& node) {
    printIndent();
    out << "Assignment: " << node.getTarget();
    if (showTypes) {
        out << " : " << node.getOp();
    }
    out << "\n";
    indentLevel++;
    node.getValue()->accept(*this);
    indentLevel--;
}
void ASTPrinter::visit(const IfNode& node) {
    printIndent();
    out << (node.getIsElif() ? "Elif" : "If") << ":\n";  // Differenzia If/Elif
    indentLevel++;

    // Stampa condizione
    printIndent();
    out << "Condition:\n";
    indentLevel++;
    node.getCondition()->accept(*this);
    indentLevel--;

    // Stampa blocco THEN
    printIndent();
    out << "Then:\n";
    indentLevel++;
    node.getThenBlock()->accept(*this);
    indentLevel-=2;

    // Gestione elif/else - MODIFICA CHIAVE
    if (const ASTNodeBase* elseBlock = node.getElseBlock().get()) {
        elseBlock->accept(*this);
    }
    indentLevel--;
}

void ASTPrinter::visit(const ElseNode& node) {
    printIndent();
    out << "Else:\n";  // Cambiato da ElseBlock a Else per consistenza
    indentLevel++;
    node.getBlock()->accept(*this);
    indentLevel--;
}
void ASTPrinter::visit(const VariableDeclNode& node) {
    Print("VariableDecl", node.getName(), node.getType());
}
void ASTPrinter::visit(const BlockNode& node) {
    Print("Block", node.getStatements());  // Stampa "Block:" e poi tutti gli statement
}
void ASTPrinter::visit(const VariableInitNode& node) {
    printIndent();
    out << "VariableInit: " << node.getName();
    if (showTypes) {
        out << " : " << node.getType();
    }
    out << "\n";
    indentLevel++;
    node.getValue()->accept(*this);
    indentLevel--;
}

// Implement other visit methods similarly...
/*
void ASTPrinter::visit(const BlockNode& node) {
    Print("Block", node.getStatements());
}
*/
/*
void ASTPrinter::visit(const IfNode& node) {
    printIndent();
    out << "If:\n";
    indentLevel++;
    printIndent();
    out << "Condition:\n";
    indentLevel++;
    node.getCondition()->accept(*this);
    indentLevel--;
    printIndent();
    out << "Then:\n";
    indentLevel++;
    node.getThenBlock()->accept(*this);
    indentLevel--;

    if (node.getElseBlock()) {
        printIndent();
        out << "Else:\n";
        indentLevel++;
        node.getElseBlock()->accept(*this);
        indentLevel--;
    }
    indentLevel--;
}
*/
// Add implementations for other node types as needed...
//
// Created by Personal on 13/06/2025.
//
#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "OLDASTVisitor.h"
#include <ostream>
#include <string>
#include <vector>

class ASTPrinter : public OLDASTVisitor {
public:
    explicit ASTPrinter(std::ostream& out, bool showTypes = false);

    // Visitor methods
    void visit(const ProgramNode& node) override;
    void visit(const IntLiteralNode& node) override;
    void visit(const FloatLiteralNode& node) override;
    void visit(const StringLiteralNode& node) override;
    void visit(const CharLiteralNode& node) override;
    void visit(const BoolLiteralNode& node) override;
    void visit(const NullLiteralNode& node) override;
    void visit(const IdentifierNode& node) override;
    void visit(const BinaryExprNode& node) override;
    void visit(const UnaryExprNode& node) override;
    void visit(const AssignmentNode& node) override;
    void visit(const VariableDeclNode& node) override;
    void visit(const VariableInitNode& node) override;
    void visit(const IfNode &) override;
    void visit(const ElseNode &) override;
    void visit(const BlockNode &) override;
    /*
    void visit(const CompoundAssignmentNode& node) override;
    void visit(const FunctionDeclNode& node) override;
    void visit(const FunctionParamNode& node) override;
    void visit(const ClassDeclNode& node) override;
    void visit(const ClassPropertyNode& node) override;
    void visit(const ClassMethodNode& node) override;
    void visit(const CallExprNode& node) override;
    void visit(const MethodCallNode& node) override;
    void visit(const ArrayAccessNode& node) override;
    void visit(const MemberAccessNode& node) override;
    void visit(const NewExprNode& node) override;
    void visit(const BlockNode& node) override;
    void visit(const IfNode& node) override;
    void visit(const ElseNode& node) override;
    void visit(const ForNode& node) override;
    void visit(const WhileNode& node) override;
    void visit(const DoWhileNode& node) override;
    void visit(const SwitchNode& node) override;
    void visit(const CaseNode& node) override;
    void visit(const ReturnNode& node) override;
    void visit(const BreakNode& node) override;
    void visit(const ContinueNode& node) override;
    void visit(const ThrowNode& node) override;
    void visit(const TryCatchNode& node) override;
    void visit(const ImportNode& node) override;
    void visit(const ExportNode& node) override;
    void visit(const TypeAnnotationNode& node) override;
    void visit(const TemplateDeclNode& node) override;
    void visit(const GenericTypeNode& node) override;
    void visit(const YieldNode& node) override;
    void visit(const AwaitNode& node) override;
    void visit(const CommentNode& node) override;
    void visit(const DirectiveNode& node) override;
*/
private:
    std::ostream& out;
    bool showTypes;
    int indentLevel = 0;

    void printIndent();
    void printWithType(const std::string& value, const std::string& type = "");
    void Print(const std::string& nodeType, const std::vector<ASTNodePtr>& children);
    void Print(const std::string& nodeType, const std::string& value, const std::string& type = "");
};

#endif // AST_PRINTER_H

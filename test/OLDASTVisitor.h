//
// Created by Personal on 13/06/2025.
//
#ifndef AST_VISITOR_H
#define AST_VISITOR_H

// Include all necessary forward declarations
#include "ASTNode.h"
class ProgramNode;
class IntLiteralNode;
class FloatLiteralNode;
class StringLiteralNode;
class CharLiteralNode;
class BoolLiteralNode;
class NullLiteralNode;
class IdentifierNode;
class BinaryExprNode;
class UnaryExprNode;
class AssignmentNode;
class VariableDeclNode;
class CompoundAssignmentNode;
class VariableInitNode;
class FunctionDeclNode;
class FunctionParamNode;
class ClassDeclNode;
class ClassPropertyNode;
class ClassMethodNode;
class CallExprNode;
class MethodCallNode;
class ArrayAccessNode;
class MemberAccessNode;
class NewExprNode;
class BlockNode;
class IfNode;
class ElseNode;
class ForNode;
class WhileNode;
class DoWhileNode;
class SwitchNode;
class CaseNode;
class ReturnNode;
class BreakNode;
class ContinueNode;
class ThrowNode;
class TryCatchNode;
class ImportNode;
class ExportNode;
class TypeAnnotationNode;
class TemplateDeclNode;
class GenericTypeNode;
class YieldNode;
class AwaitNode;
class CommentNode;
class DirectiveNode;

class OLDASTVisitor {
public:
    virtual ~OLDASTVisitor() = default;
    virtual void visit(const ProgramNode&) {}
    virtual void visit(const IntLiteralNode&) {}
    virtual void visit(const FloatLiteralNode&) {}
    virtual void visit(const StringLiteralNode&) {}
    virtual void visit(const CharLiteralNode&) {}
    virtual void visit(const BoolLiteralNode&) {}
    virtual void visit(const NullLiteralNode&){}
    virtual void visit(const IdentifierNode&) {}
    virtual void visit(const BinaryExprNode&) {}
    virtual void visit(const UnaryExprNode&) {}
    virtual void visit(const AssignmentNode&) {}
    virtual void visit(const VariableDeclNode&) {}
    virtual void visit(const CompoundAssignmentNode&) {}
    virtual void visit(const VariableInitNode&) {}
    virtual void visit(const FunctionDeclNode&) {}
    virtual void visit(const FunctionParamNode&) {}
    virtual void visit(const ClassDeclNode&) {}
    virtual void visit(const ClassPropertyNode&) {}
    virtual void visit(const ClassMethodNode&) {}
    virtual void visit(const CallExprNode&) {}
    virtual void visit(const MethodCallNode&) {}
    virtual void visit(const ArrayAccessNode&) {}
    virtual void visit(const MemberAccessNode&) {}
    virtual void visit(const NewExprNode&) {}
    virtual void visit(const BlockNode&) {}
    virtual void visit(const IfNode&) {}
    virtual void visit(const ElseNode&) {}
    virtual void visit(const ForNode&) {}
    virtual void visit(const WhileNode&) {}
    virtual void visit(const DoWhileNode&) {}
    virtual void visit(const SwitchNode&) {}
    virtual void visit(const CaseNode&) {}
    virtual void visit(const ReturnNode&) {}
    virtual void visit(const BreakNode&) {}
    virtual void visit(const ContinueNode&) {}
    virtual void visit(const ThrowNode&) {}
    virtual void visit(const TryCatchNode&) {}
    virtual void visit(const ImportNode&) {}
    virtual void visit(const ExportNode&) {}
    virtual void visit(const TypeAnnotationNode&) {}
    virtual void visit(const TemplateDeclNode&) {}
    virtual void visit(const GenericTypeNode&) {}
    virtual void visit(const YieldNode&) {}
    virtual void visit(const AwaitNode&) {}
    virtual void visit(const CommentNode&) {}
    virtual void visit(const DirectiveNode&) {}
};

#endif // AST_VISITOR_H
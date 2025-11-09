//
// Created by Cristian on 09/11/2025.
//
#pragma once
// Forward declarations di tutti i nodi
class ProgramNode;
class VariableDeclarationNode;
class FunctionDeclarationNode;
class ParameterNode;
class BlockStatementNode;
class IfStatementNode;
class WhileStatementNode;
class ForStatementNode;
class ReturnStatementNode;
class AssignmentNode;
class BinaryOperationNode;
class UnaryOperationNode;
class FunctionCallNode;
class VariableNode;
class LiteralNode;
class TypeReferenceNode;

class Visitor {
public:
    virtual ~Visitor() = default;

    // Dichiarazioni di visit per tutti i tipi di nodo
    virtual void visit(const ProgramNode& node) = 0;
    virtual void visit(const VariableDeclarationNode& node) = 0;
    virtual void visit(const FunctionDeclarationNode& node) = 0;
    virtual void visit(const ParameterNode& node) = 0;
    virtual void visit(const BlockStatementNode& node) = 0;
    virtual void visit(const IfStatementNode& node) = 0;
    virtual void visit(const WhileStatementNode& node) = 0;
    virtual void visit(const ForStatementNode& node) = 0;
    virtual void visit(const ReturnStatementNode& node) = 0;
    virtual void visit(const AssignmentNode& node) = 0;
    virtual void visit(const BinaryOperationNode& node) = 0;
    virtual void visit(const UnaryOperationNode& node) = 0;
    virtual void visit(const FunctionCallNode& node) = 0;
    virtual void visit(const VariableNode& node) = 0;
    virtual void visit(const LiteralNode& node) = 0;
    virtual void visit(const TypeReferenceNode& node) = 0;
};
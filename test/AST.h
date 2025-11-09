#ifndef AST_H
#define AST_H

#include "../include/AST/ASTNode.h"

// ======================================================
// Additional utilities and forward declarations
// ======================================================
enum class NodeType {
    // Dichiarazioni
    Program,
    VariableDecl,
    FunctionDecl,
    ClassDecl,

    // Espressioni
    BinaryExpr,
    UnaryExpr,
    CallExpr,
    Identifier,

    // Letterali
    IntLiteral,
    FloatLiteral,
    StringLiteral,

    // Statement
    Block,
    If,
    For,
    While,
    Return,

    // Altri
    Import,
    TypeAnnotation
};

// Helper per nodi che necessitano di NodeType
template <typename Derived, NodeType Type>
class TypedASTNode : public ASTNode<Derived> {
public:
    NodeType getType() const { return Type; }
};

#endif // AST_H
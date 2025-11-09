#ifndef ASTNODE_H
#define ASTNODE_H

#include <memory>
#include <variant>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "OLDASTVisitor.h"
// Forward declare ASTVisitor here
class OLDASTVisitor;

class ASTNodeBase {
public:
    virtual ~ASTNodeBase() = default;
    virtual void accept(OLDASTVisitor& visitor) const = 0;
};
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

// Define unique_ptr aliases
#define DEFINE_PTR(type) using type##Ptr = std::unique_ptr<type>;
DEFINE_PTR(ProgramNode)
DEFINE_PTR(IntLiteralNode)
DEFINE_PTR(FloatLiteralNode)
DEFINE_PTR(StringLiteralNode)
DEFINE_PTR(CharLiteralNode)
DEFINE_PTR(BoolLiteralNode)
DEFINE_PTR(NullLiteralNode)
DEFINE_PTR(IdentifierNode)
DEFINE_PTR(BinaryExprNode)
DEFINE_PTR(UnaryExprNode)
DEFINE_PTR(AssignmentNode)
DEFINE_PTR(VariableDeclNode)
DEFINE_PTR(CompoundAssignmentNode)
DEFINE_PTR(VariableInitNode)
DEFINE_PTR(FunctionDeclNode)
DEFINE_PTR(FunctionParamNode)
DEFINE_PTR(ClassDeclNode)
DEFINE_PTR(ClassPropertyNode)
DEFINE_PTR(ClassMethodNode)
DEFINE_PTR(CallExprNode)
DEFINE_PTR(MethodCallNode)
DEFINE_PTR(ArrayAccessNode)
DEFINE_PTR(MemberAccessNode)
DEFINE_PTR(NewExprNode)
DEFINE_PTR(BlockNode)
DEFINE_PTR(IfNode)
DEFINE_PTR(ElseNode)
DEFINE_PTR(ForNode)
DEFINE_PTR(WhileNode)
DEFINE_PTR(DoWhileNode)
DEFINE_PTR(SwitchNode)
DEFINE_PTR(CaseNode)
DEFINE_PTR(ReturnNode)
DEFINE_PTR(BreakNode)
DEFINE_PTR(ContinueNode)
DEFINE_PTR(ThrowNode)
DEFINE_PTR(TryCatchNode)
DEFINE_PTR(ImportNode)
DEFINE_PTR(ExportNode)
DEFINE_PTR(TypeAnnotationNode)
DEFINE_PTR(TemplateDeclNode)
DEFINE_PTR(GenericTypeNode)
DEFINE_PTR(YieldNode)
DEFINE_PTR(AwaitNode)
DEFINE_PTR(CommentNode)
DEFINE_PTR(DirectiveNode)
#undef DEFINE_PTR

// Variant type for AST nodes
using ASTNodeVariant = std::variant<
    ProgramNode*,
    IntLiteralNode*,
    FloatLiteralNode*,
    StringLiteralNode*,
    CharLiteralNode*,
    BoolLiteralNode*,
    NullLiteralNode*,
    IdentifierNode*,
    BinaryExprNode*,
    UnaryExprNode*,
    AssignmentNode*,
    VariableDeclNode*,
    CompoundAssignmentNode*,
    VariableInitNode*,
    FunctionDeclNode*,
    FunctionParamNode*,
    ClassDeclNode*,
    ClassPropertyNode*,
    ClassMethodNode*,
    CallExprNode*,
    MethodCallNode*,
    ArrayAccessNode*,
    MemberAccessNode*,
    NewExprNode*,
    BlockNode*,
    IfNode*,
    ElseNode*,
    ForNode*,
    WhileNode*,
    DoWhileNode*,
    SwitchNode*,
    CaseNode*,
    ReturnNode*,
    BreakNode*,
    ContinueNode*,
    ThrowNode*,
    TryCatchNode*,
    ImportNode*,
    ExportNode*,
    TypeAnnotationNode*,
    TemplateDeclNode*,
    GenericTypeNode*,
    YieldNode*,
    AwaitNode*,
    CommentNode*,
    DirectiveNode*
>;

// Now define ASTNode template after all forward declarations
template <typename Derived>
class ASTNode : public ASTNodeBase {
public:
    // Version non-const
    void accept(OLDASTVisitor& visitor) {
        visitor.visit(static_cast<Derived&>(*this));
    }

    // Version const
    void accept(OLDASTVisitor& visitor) const {
        visitor.visit(static_cast<const Derived&>(*this));
    }
};

using ASTNodePtr = std::unique_ptr<ASTNodeBase>;

// Macro for defining AST node classes (to be used in their own headers)
#define AST_NODE(type) \
class type : public ASTNode<type>

#endif // ASTNODE_H
//
// Created by Personal on 29/06/2025.
//
#include "AST/ASTNote.h"

template <typename Derived, NodeType Type>
class TypedASTNode : public ASTNode {
public:
    NodeType getType() const override {
        return Type;
    }

    void accept(Visitor& visitor) const override {
        visitor.visit(static_cast<const Derived&>(*this));
    }

    std::unique_ptr<ASTNode> clone() const override {
        return std::make_unique<Derived>(*static_cast<const Derived*>(this));
    }

    std::string toString() const override {
        return static_cast<const Derived*>(this)->toStringImpl();
    }

protected:
    // Metodo helper per validazioni comuni
    void validate() const {
        // Validazione di base per tutti i nodi
    }
};
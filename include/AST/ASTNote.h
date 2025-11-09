//
// Created by Cristian on 09/11/2025.
//
#pragma once

#include "Visitor.h"
#include "NodeType.h"
#include <memory>
#include <vector>

// Interfaccia base per tutti i nodi AST
class ASTNode {
public:
    virtual ~ASTNode() = default;

    // Metodi virtuali puri che tutti i nodi devono implementare
    virtual NodeType getType() const = 0;
    virtual void accept(Visitor& visitor) const = 0;
    virtual std::unique_ptr<ASTNode> clone() const = 0;

    // Metodo helper per debugging
    virtual std::string toString() const = 0;
};

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
//
// Created by Cristian on 09/11/2025.
//
#pragma once

#include "Visitor.h"
#include "NodeType.h"
#include <memory>

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

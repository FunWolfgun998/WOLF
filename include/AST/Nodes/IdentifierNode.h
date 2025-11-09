//
// Created by Personal on 28/06/2025.
//

#ifndef IDENTIFIERNODE_H
#define IDENTIFIERNODE_H

#include "../../../test/ASTNode.h"
#include <string>

AST_NODE(IdentifierNode) {
    std::string value;

    public:
    explicit IdentifierNode(std::string name) : value(std::move(name)) {}
    const std::string getValue() const { return value; }


};

#endif //IDENTIFIERNODE_H

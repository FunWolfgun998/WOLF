//
// Created by Personal on 28/06/2025.
//

#ifndef BOOTLITERALNODE_H
#define BOOTLITERALNODE_H


#include "../../../test/ASTNode.h"

AST_NODE(BoolLiteralNode) {
    bool value;
    public:
    explicit BoolLiteralNode(bool value): value(value){}
    const bool getValue() const { return value; }


};
#endif //BOOTLITERALNODE_H

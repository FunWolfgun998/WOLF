//
// Created by Personal on 27/06/2025.
//

#ifndef FLOATLITERALNODE_H
#define FLOATLITERALNODE_H
#include "../../../test/ASTNode.h"

AST_NODE(FloatLiteralNode) {
    float value;
    public:
    explicit FloatLiteralNode(float value): value(value){}
    const float getValue() const { return value; }

    
};

#endif //FLOATLITERALNODE_H

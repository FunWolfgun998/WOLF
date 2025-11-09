//
// Created by Personal on 04/07/2025.
//

#ifndef ELSENODE_H
#define ELSENODE_H

#include "../../../test/ASTNode.h"

AST_NODE(ElseNode){
    //The call that I'm creating is taking as base the base ASTNode and as parameter is taking itself
    //I add the parameter that the node need to save inside itself
    ASTNodePtr  block;
    public:
    //I create the constructor to generate the node with the arguments that I need
    explicit ElseNode(ASTNodePtr block):
        block(std::move(block)){}

    const ASTNodePtr& getBlock() const { return block; }

};

#endif //ELSENODE_H

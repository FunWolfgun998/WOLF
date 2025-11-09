//
// Created by Personal on 27/06/2025.
//

#ifndef BINARYEXPRNODE_H
#define BINARYEXPRNODE_H

#include "../../../test/ASTNode.h"

AST_NODE(BinaryExprNode){
    //The call that I'm creating is taking as base the base ASTNode and as parameter is taking itself
    //I add the parameter that the node need to save inside itself
    ASTNodePtr left;
    std::string op;
    ASTNodePtr  right;
    public:
    //I create the constructor to generate the node with the arguments that I need
    explicit BinaryExprNode( ASTNodePtr left, std::string op, ASTNodePtr right):
        left(std::move(left)),op(std::move(op)),right(std::move(right)){}

    //the syntax constructor (input1,input2...): var1(what to save in var1 ex. std::move(input1),var2()...{}
    const std::string getOp() const { return op; }
    const ASTNodePtr& getLeft() const { return left; }
    const ASTNodePtr& getRight() const { return right; }

};

#endif //BinaryExprNode_H

//
// Created by Personal on 28/06/2025.
//

#ifndef UNARYEXPRNODE_H
#define UNARYEXPRNODE_H

#include "../../../test/ASTNode.h"
#include <string>

AST_NODE(UnaryExprNode) {
    std::string op;       // Unary operator (e.g. "-", "!", "++")
    ASTNodePtr operand;   // The expression to which the operator applies

    public:
    explicit UnaryExprNode(std::string op, ASTNodePtr operand)
        : op(std::move(op)), operand(std::move(operand)) {}
    const std::string getOp() const { return op; }
    const ASTNodePtr& getOperand() const { return operand; }

    /*
    const std::string& getOp() const { return op; }
    const ASTNode& getOperand() const { return *operand; }
    */
};

#endif //UNARYEXPRNODE_H

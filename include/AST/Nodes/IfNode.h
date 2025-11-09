//
// Created by Personal on 04/07/2025.
//

#ifndef IFNODE_H
#define IFNODE_H

#include "../../../test/ASTNode.h"

AST_NODE(IfNode) {
    ASTNodePtr condition;
    ASTNodePtr thenBlock;
    ASTNodePtr elseBlock;
    bool isElif;

    public:
    IfNode(ASTNodePtr condition, ASTNodePtr thenBlock, ASTNodePtr elseBlock = nullptr, bool isElif = false)
        : condition(std::move(condition)),
          thenBlock(std::move(thenBlock)),
          elseBlock(std::move(elseBlock)),
          isElif(isElif){}

    const ASTNodePtr& getCondition() const { return condition; }
    const ASTNodePtr& getThenBlock() const { return thenBlock; }
    const ASTNodePtr& getElseBlock() const { return elseBlock; }
    bool hasElse() const { return elseBlock != nullptr; }
    bool getIsElif() const { return isElif; }
};

#endif //IFNODE_H

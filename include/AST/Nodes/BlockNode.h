//
// Created by Personal on 04/07/2025.
//

#ifndef BLOCKNODE_H
#define BLOCKNODE_H

#include "../../../test/ASTNode.h"
AST_NODE(BlockNode) {
    std::vector<ASTNodePtr> statements;

    public:
    /*
     explicit is needed to be sure that implicit conversions aren't allowed.
     ProgramNode p = ProgramNode(statements) OK, ProgramNode p = var1 NO ERROR
     */
    explicit BlockNode(std::vector<ASTNodePtr> statements)
        : statements(std::move(statements)) {}

    const std::vector<ASTNodePtr>& getStatements() const { return statements; }


};

#endif //BLOCKNODE_H

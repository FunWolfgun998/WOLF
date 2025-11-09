//
// Created by Personal on 28/06/2025.
//

#ifndef ASSIGNMENTNODE_H
#define ASSIGNMENTNODE_H

#include "../../../test/ASTNode.h"
AST_NODE(AssignmentNode) {
    std::string target;    // L-value (normally IdentifierNode or MemberAccessNode)
    ASTNodePtr value;     // R-value (expression to assign)
    std::string op;       // Operator ("=", "+=", "-=", etc.)

    public:
    AssignmentNode(std::string target, std::string op, ASTNodePtr value)
        : target(std::move(target)), op(op), value(std::move(value)) {}

    const std::string getTarget() const { return target; }
    const std::string getOp() const { return op; }
    const ASTNodePtr& getValue() const { return value; }

};

#endif //ASSIGNMENTNODE_H

//
// Created by Personal on 28/06/2025.
//

#ifndef VARIABLEDECLNODE_H
#define VARIABLEDECLNODE_H
#include "../../../test/ASTNode.h"

AST_NODE(VariableDeclNode) {
    std::string name;           // Nome della variabile (es. "x")
    std::string type;     // Type es. int, float, char,

    public:
    explicit VariableDeclNode(std::string name,std::string type)
    : name(std::move(name)),type(std::move(type)){}

    const std::string& getName() const { return name; }
    const std::string& getType() const { return type; }

};
#endif //VARIABLEDECLNODE_H

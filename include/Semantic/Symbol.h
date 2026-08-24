#pragma once

#include <string>
#include "Type.h"
#include "../Lexer/Token.h"
#include "../AST/ASTNodes.h"

enum class SymbolKind {
    VARIABLE,
    PARAMETER,
    FUNCTION,
    STRUCT,
    CLASS,
    FIELD,
    METHOD
};

struct Symbol {
    std::string name = "";
    const Type* type = nullptr;
    SymbolKind kind = SymbolKind::VARIABLE;
    AccessModifier access = AccessModifier::PUBLIC;

    // Safety flags
    bool isConstant = false;
    bool isInitialized = true;

    Token token; // Coordinates for error diagnostics

    Symbol() = default;
    Symbol(std::string name, const Type* type, SymbolKind kind, Token token, AccessModifier access = AccessModifier::PUBLIC)
        : name(std::move(name)), type(type), kind(kind), access(access), token(std::move(token)) {}
};
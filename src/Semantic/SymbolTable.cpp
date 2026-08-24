#include "../../include/Semantic/SymbolTable.h"

// Scope methods
bool Scope::define(const Symbol& symbol) {
    if (symbols.find(symbol.name) != symbols.end()) {
        return false; // Redeclaration error in same scope
    }
    symbols[symbol.name] = symbol;
    return true;
}

std::optional<Symbol> Scope::resolve(const std::string& name) const {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return it->second;
    }
    if (parent != nullptr) {
        return parent->resolve(name); // Search up the scope hierarchy
    }
    return std::nullopt;
}

bool Scope::existsInCurrent(const std::string& name) const {
    return symbols.find(name) != symbols.end();
}

// SymbolTable constructor: Pure global scope without hardcoded built-ins
SymbolTable::SymbolTable() {
    globalScope = std::make_shared<Scope>(ScopeKind::GLOBAL, nullptr);
    currentScope = globalScope;
}

void SymbolTable::enterScope(ScopeKind kind) {
    currentScope = std::make_shared<Scope>(kind, currentScope);
}

void SymbolTable::exitScope() {
    if (currentScope->getParent() != nullptr) {
        currentScope = currentScope->getParent();
    }
}

bool SymbolTable::define(const Symbol& symbol) {
    return currentScope->define(symbol);
}

std::optional<Symbol> SymbolTable::resolve(const std::string& name) const {
    return currentScope->resolve(name);
}

bool SymbolTable::existsInCurrentScope(const std::string& name) const {
    return currentScope->existsInCurrent(name);
}

bool SymbolTable::isInsideLoop() const {
    auto scope = currentScope;
    while (scope != nullptr) {
        if (scope->getKind() == ScopeKind::BLOCK) return true;
        if (scope->getKind() == ScopeKind::FUNCTION) break;
        scope = scope->getParent();
    }
    return false;
}
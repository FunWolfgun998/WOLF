#include "../../include/Semantic/SymbolTable.h"

// --- SCOPE IMPLEMENTATION ---

bool Scope::define(const Symbol& symbol) {
    if (symbols.find(symbol.name) != symbols.end()) {
        return false; // Error: Symbol redefinition in the same scope
    }
    symbols[symbol.name] = symbol;
    return true;
}

std::optional<Symbol> Scope::resolve(const std::string& name) const {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return it->second; // Found in current scope
    }
    
    // Look up recursively in parent scope
    if (parent != nullptr) {
        return parent->resolve(name);
    }
    
    return std::nullopt; // Not found anywhere
}

bool Scope::existsInCurrent(const std::string& name) const {
    return symbols.find(name) != symbols.end();
}

// --- SYMBOL TABLE IMPLEMENTATION ---

SymbolTable::SymbolTable() {
    // Initialize the global scope at start
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
        if (scope->getKind() == ScopeKind::BLOCK) {
            // Block scopes inside while/for loops
            return true;
        }
        if (scope->getKind() == ScopeKind::FUNCTION) {
            break; // Stop looking beyond function boundaries
        }
        scope = scope->getParent();
    }
    return false;
}
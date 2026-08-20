#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>
#include "Symbol.h"

enum class ScopeKind {
    GLOBAL,
    CLASS,
    FUNCTION,
    BLOCK
};

class Scope {
private:
    ScopeKind kind;
    std::shared_ptr<Scope> parent;
    std::unordered_map<std::string, Symbol> symbols;

public:
    explicit Scope(ScopeKind kind, std::shared_ptr<Scope> parent = nullptr)
        : kind(kind), parent(parent) {}

    ScopeKind getKind() const { return kind; }
    std::shared_ptr<Scope> getParent() const { return parent; }

    // Define a symbol strictly in this scope (returns false if already defined)
    bool define(const Symbol& symbol);

    // Resolve a symbol looking in this scope, and if not found, recursively in parent scopes
    std::optional<Symbol> resolve(const std::string& name) const;

    // Check if symbol exists only in the current scope (to detect duplicate variable declarations)
    bool existsInCurrent(const std::string& name) const;

    const std::unordered_map<std::string, Symbol>& getAllSymbols() const { return symbols; }
};

class SymbolTable {
private:
    std::shared_ptr<Scope> currentScope;
    std::shared_ptr<Scope> globalScope;

public:
    SymbolTable();

    // Push a new scope (when entering a block, function, or class)
    void enterScope(ScopeKind kind);

    // Pop the current scope (when exiting a block, function, or class)
    void exitScope();

    // Define a symbol in the current active scope
    bool define(const Symbol& symbol);

    // Look up a symbol from current scope up to global scope
    std::optional<Symbol> resolve(const std::string& name) const;

    // Check if symbol exists in the current scope
    bool existsInCurrentScope(const std::string& name) const;

    std::shared_ptr<Scope> getCurrentScope() const { return currentScope; }
    std::shared_ptr<Scope> getGlobalScope() const { return globalScope; }

    // Check if currently inside a loop (useful for break/continue validation)
    bool isInsideLoop() const;
};
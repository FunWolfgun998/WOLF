#include "../../include/Semantic/SemanticAnalyzer.h"
#include <iostream>
#include <unordered_set>

void SemanticAnalyzer::reportError(DiagnosticCode code, const Token& token, const std::string& message, const std::string& hint) {
    // Convert single token to SourceSpan and forward to diagnostic engine
    diagnostics.report(code, SourceSpan(token), message, hint);
}

void SemanticAnalyzer::reportError(DiagnosticCode code, const SourceSpan& span, const std::string& message, const std::string& hint) {
    // Forward computed source span directly to diagnostic engine
    diagnostics.report(code, span, message, hint);
}

void SemanticAnalyzer::reportMultiSpan(DiagnosticCode code, std::string message, const std::vector<SpanLabel>& labels, std::string hint) {
    // Forward multiple source labels to diagnostic engine
    diagnostics.reportMultiSpan(code, std::move(message), labels, std::move(hint));
}

std::vector<std::string> SemanticAnalyzer::getVisibleSymbolNames() const {
    std::vector<std::string> names;
    std::unordered_set<std::string> seen;
    auto scope = symbolTable.getCurrentScope();

    // Traverse active lexical scope chain upwards and filter duplicate shadowed names
    while (scope != nullptr) {
        for (const auto& [name, sym] : scope->getAllSymbols()) {
            // Include only variables, parameters, and functions for expression typo matches
            if (sym.kind != SymbolKind::CLASS && sym.kind != SymbolKind::STRUCT) {
                if (seen.insert(name).second) {
                    names.push_back(name);
                }
            }
        }
        scope = scope->getParent();
    }
    return names;
}
std::vector<std::string> SemanticAnalyzer::getVisibleTypeNames() const {
    // Start with all standard built-in primitive type names
    std::vector<std::string> typeNames = {"int", "float", "char", "string", "bool", "void"};
    std::unordered_set<std::string> seen(typeNames.begin(), typeNames.end());
    auto scope = symbolTable.getCurrentScope();

    // Collect all class and struct type definitions currently in scope
    while (scope != nullptr) {
        for (const auto& [name, sym] : scope->getAllSymbols()) {
            if (sym.kind == SymbolKind::CLASS || sym.kind == SymbolKind::STRUCT) {
                if (seen.insert(name).second) {
                    typeNames.push_back(name);
                }
            }
        }
        scope = scope->getParent();
    }
    return typeNames;
}

const Type* SemanticAnalyzer::resolveTypeFromToken(const Token& typeToken) {
    std::string typeName = typeToken.lexeme;

    // 1. Handle outermost nullable modifier '?' first (e.g. "int?", "int[]?", "int[][]?")
    if (typeName.size() > 1 && typeName.back() == '?') {
        Token baseToken = typeToken;
        baseToken.lexeme = typeName.substr(0, typeName.size() - 1);
        const Type* baseType = resolveTypeFromToken(baseToken);
        return Type::makeNullable(baseType);
    }

    // 2. Handle array dimensions '[]' (e.g. "int[]", "int[][]", "Dog[]")
    if (typeName.size() > 2 && typeName.substr(typeName.size() - 2) == "[]") {
        Token baseToken = typeToken;
        baseToken.lexeme = typeName.substr(0, typeName.size() - 2);
        const Type* baseType = resolveTypeFromToken(baseToken);
        return Type::makeArray(baseType);
    }

    // 3. Built-in primitive types
    if (typeName == "int") return Type::getInt();
    if (typeName == "float") return Type::getFloat();
    if (typeName == "char") return Type::getChar();
    if (typeName == "string") return Type::getString();
    if (typeName == "bool") return Type::getBool();
    if (typeName == "void") return Type::getVoid();

    // 4. Look up user-defined types (classes and structs) in the symbol table
    auto symbolOpt = symbolTable.resolve(typeName);
    if (symbolOpt.has_value()) {
        if (symbolOpt->kind == SymbolKind::CLASS || symbolOpt->kind == SymbolKind::STRUCT) {
            return symbolOpt->type;
        }
        reportError(DiagnosticCode::E0007_UNKNOWN_TYPE_NAME, typeToken,
                    "'" + typeName + "' is a variable name, not a type.",
                    "variables cannot be used in type positions");
        return Type::getError();
    }

    // Use dedicated helper to find matching type candidates without scope iteration duplication
    std::string suggestion = diagnostics.findClosestMatch(typeName, getVisibleTypeNames());
    std::string hint = suggestion.empty() ? "ensure the class or struct is declared in scope" : "did you mean '" + suggestion + "'?";
    reportError(DiagnosticCode::E0007_UNKNOWN_TYPE_NAME, typeToken, "Unknown type name '" + typeName + "'.", hint);
    return Type::getError();
}

bool SemanticAnalyzer::isLValue(const Expr& expr) {
    // Only variables, member accesses, and array subscripts denote assignable memory locations
    return std::holds_alternative<std::unique_ptr<VariableExpr>>(expr.as) ||
           std::holds_alternative<std::unique_ptr<MemberAccessExpr>>(expr.as) ||
           std::holds_alternative<std::unique_ptr<ArrayAccessExpr>>(expr.as);
}

Token SemanticAnalyzer::getExprToken(const Expr& expr) {
    return std::visit([](const auto& node) -> Token {
        using T = std::decay_t<decltype(node)>;
        // Null safety guard against uninitialized or empty variant pointers
        if (!node) return Token(TokenType::UNKNOWN, "", 0, 0);

        if constexpr (std::is_same_v<T, std::unique_ptr<LiteralExpr>>) return node->value;
        if constexpr (std::is_same_v<T, std::unique_ptr<VariableExpr>>) return node->name;
        if constexpr (std::is_same_v<T, std::unique_ptr<ThisExpr>>) return node->keyword;
        if constexpr (std::is_same_v<T, std::unique_ptr<BaseExpr>>) return node->keyword;
        if constexpr (std::is_same_v<T, std::unique_ptr<CastExpr>>) return node->asToken;
        if constexpr (std::is_same_v<T, std::unique_ptr<BinaryExpr>>) return node->op;
        if constexpr (std::is_same_v<T, std::unique_ptr<RangeExpr>>) return node->op;
        if constexpr (std::is_same_v<T, std::unique_ptr<UnaryExpr>>) return node->op;
        if constexpr (std::is_same_v<T, std::unique_ptr<GroupingExpr>>) return node->openingParen;
        if constexpr (std::is_same_v<T, std::unique_ptr<CallExpr>>) return node->paren;
        if constexpr (std::is_same_v<T, std::unique_ptr<MemberAccessExpr>>) return node->member;
        if constexpr (std::is_same_v<T, std::unique_ptr<ArrayAccessExpr>>) return node->openingBracket;
        if constexpr (std::is_same_v<T, std::unique_ptr<ArrayLiteralExpr>>) return node->openingBracket;
        if constexpr (std::is_same_v<T, std::unique_ptr<TernaryExpr>>) return node->questionMark;
        return Token(TokenType::UNKNOWN, "", 0, 0);
    }, expr.as);
}

SourceSpan SemanticAnalyzer::getExprSpan(const Expr& expr) {
    return SourceSpan(getExprToken(expr));
}

Token SemanticAnalyzer::getStmtToken(const Stmt& stmt) {
    return std::visit([](const auto& node) -> Token {
        using T = std::decay_t<decltype(node)>;
        // Null safety guard against uninitialized or empty statement pointers
        if (!node) return Token(TokenType::UNKNOWN, "", 0, 0);

        if constexpr (std::is_same_v<T, std::unique_ptr<ExpressionStmt>>) return getExprToken(node->expression);
        if constexpr (std::is_same_v<T, std::unique_ptr<VarDeclStmt>>) return node->name;
        if constexpr (std::is_same_v<T, std::unique_ptr<BlockStmt>>) {
            if (!node->statements.empty()) return getStmtToken(node->statements[0]);
            return Token(TokenType::UNKNOWN, "", 0, 0);
        }
        if constexpr (std::is_same_v<T, std::unique_ptr<IfStmt>>) return node->keyword;
        if constexpr (std::is_same_v<T, std::unique_ptr<WhileStmt>>) return node->keyword;
        if constexpr (std::is_same_v<T, std::unique_ptr<ForStmt>>) return node->keyword;
        if constexpr (std::is_same_v<T, std::unique_ptr<FunctionDeclStmt>>) return node->name;
        if constexpr (std::is_same_v<T, std::unique_ptr<StructDeclStmt>>) return node->name;
        if constexpr (std::is_same_v<T, std::unique_ptr<ClassDeclStmt>>) return node->name;
        if constexpr (std::is_same_v<T, std::unique_ptr<ReturnStmt>>) return node->keyword;
        if constexpr (std::is_same_v<T, std::unique_ptr<BreakStmt>>) return node->keyword;
        if constexpr (std::is_same_v<T, std::unique_ptr<ContinueStmt>>) return node->keyword;
        return Token(TokenType::UNKNOWN, "", 0, 0);
    }, stmt.as);
}

SourceSpan SemanticAnalyzer::getStmtSpan(const Stmt& stmt) {
    return SourceSpan(getStmtToken(stmt));
}

bool SemanticAnalyzer::hasGuaranteedReturn(const Stmt& stmt) {
    return std::visit([this](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if (!node) return false;

        // 1. Direct return statement guarantees termination
        if constexpr (std::is_same_v<T, std::unique_ptr<ReturnStmt>>) {
            return true;
        }

        // 2. Block statement terminates if any sequential statement terminates
        if constexpr (std::is_same_v<T, std::unique_ptr<BlockStmt>>) {
            for (size_t i = 0; i < node->statements.size(); ++i) {
                if (hasGuaranteedReturn(node->statements[i])) {
                    // Check for unreachable dead code following the terminating statement
                    if (i + 1 < node->statements.size()) {
                        diagnostics.reportWarning(
                            DiagnosticCode::W0003_UNREACHABLE_CODE,
                            getStmtSpan(node->statements[i + 1]),
                            "Unreachable code detected after terminating return statement.",
                            "remove or restructure statements following the return"
                        );
                    }
                    return true;
                }
            }
            return false;
        }

        // 3. Conditional statement terminates only if ALL branches terminate
        if constexpr (std::is_same_v<T, std::unique_ptr<IfStmt>>) {
            // An if without an else branch cannot guarantee a return on all paths
            if (node->elseBranch == nullptr) return false;

            if (!hasGuaranteedReturn(node->thenBranch)) return false;

            for (const auto& elifBranch : node->elifBranches) {
                if (!hasGuaranteedReturn(elifBranch.block)) return false;
            }

            return hasGuaranteedReturn(*node->elseBranch);
        }

        return false;
    }, stmt.as);
}

const Type* SemanticAnalyzer::evaluate(const Expr& expr) {
    // Dispatch expression visitor via std::visit
    return std::visit(*this, expr.as);
}

void SemanticAnalyzer::execute(const Stmt& stmt) {
    // Dispatch statement visitor via std::visit
    std::visit(*this, stmt.as);
}

bool SemanticAnalyzer::analyze(const std::vector<Stmt>& ast) {
    diagnostics.clear();

    // Pass 1: Forward-declare types and function signatures
    collectDeclarations(ast);

    // Pass 2: Type-check bodies and validate expressions
    typeCheck(ast);

    return !hasErrors();
}

void SemanticAnalyzer::collectDeclarations(const std::vector<Stmt>& ast) {
    // First sub-pass: Forward-declare all struct and class type identities
    for (const auto& stmt : ast) {
        if (auto structStmt = std::get_if<std::unique_ptr<StructDeclStmt>>(&stmt.as)) {
            auto structType = std::make_unique<Type>();
            structType->kind = TypeKind::STRUCT;
            structType->name = (*structStmt)->name.lexeme;

            const Type* ptr = structType.get();
            customTypes.push_back(std::move(structType));

            if (!symbolTable.define(Symbol((*structStmt)->name.lexeme, ptr, SymbolKind::STRUCT, (*structStmt)->name))) {
                reportError(DiagnosticCode::E0004_DUPLICATE_DECLARATION, (*structStmt)->name,
                            "Redefinition of struct '" + (*structStmt)->name.lexeme + "'.",
                            "struct names must be unique within global scope");
            }
        }
        else if (auto classStmt = std::get_if<std::unique_ptr<ClassDeclStmt>>(&stmt.as)) {
            auto classType = std::make_unique<Type>();
            classType->kind = TypeKind::CLASS;
            classType->name = (*classStmt)->name.lexeme;

            const Type* ptr = classType.get();
            customTypes.push_back(std::move(classType));

            if (!symbolTable.define(Symbol((*classStmt)->name.lexeme, ptr, SymbolKind::CLASS, (*classStmt)->name))) {
                reportError(DiagnosticCode::E0004_DUPLICATE_DECLARATION, (*classStmt)->name,
                            "Redefinition of class '" + (*classStmt)->name.lexeme + "'.",
                            "class names must be unique within global scope");
            }
        }
    }

    // Second sub-pass: Resolve member layouts, inheritance links, and global function prototypes
    for (const auto& stmt : ast) {
        // Populate struct fields
        if (auto structStmt = std::get_if<std::unique_ptr<StructDeclStmt>>(&stmt.as)) {
            auto symOpt = symbolTable.resolve((*structStmt)->name.lexeme);
            if (symOpt.has_value()) {
                Type* structType = const_cast<Type*>(symOpt->type);
                for (const auto& f : (*structStmt)->fields) {
                    if (auto varDecl = std::get_if<std::unique_ptr<VarDeclStmt>>(&f.as)) {
                        const std::string& fieldName = (*varDecl)->name.lexeme;

                        // Reject duplicate fields inside the same struct
                        if (structType->fields.find(fieldName) != structType->fields.end()) {
                            reportError(DiagnosticCode::E0004_DUPLICATE_DECLARATION, (*varDecl)->name,
                                        "Duplicate field '" + fieldName + "' in struct '" + structType->name + "'.");
                            continue;
                        }

                        const Type* fType = resolveTypeFromToken((*varDecl)->type);
                        structType->fields[fieldName] = MemberInfo{fType, AccessModifier::PUBLIC};
                    }
                }
            }
        }
        // Populate class inheritance, fields, and methods
        else if (auto classStmt = std::get_if<std::unique_ptr<ClassDeclStmt>>(&stmt.as)) {
            auto symOpt = symbolTable.resolve((*classStmt)->name.lexeme);
            if (symOpt.has_value()) {
                Type* classType = const_cast<Type*>(symOpt->type);

                // Resolve superclass with cycle detection
                if ((*classStmt)->superclass.has_value()) {
                    auto parentSym = symbolTable.resolve((*classStmt)->superclass->lexeme);
                    if (parentSym.has_value() && parentSym->kind == SymbolKind::CLASS) {
                        const Type* candidate = parentSym->type;

                        // Traverse parent chain to prevent cyclic inheritance loops (e.g. A extends B, B extends A)
                        bool cyclic = false;
                        const Type* curr = candidate;
                        while (curr != nullptr) {
                            if (curr == classType) {
                                cyclic = true;
                                break;
                            }
                            curr = curr->superclass;
                        }
                        if (cyclic) {
                            reportError(DiagnosticCode::E0035_CYCLIC_INHERITANCE, *(*classStmt)->superclass,
                                        "Cyclic inheritance detected: class '" + classType->name + "' cannot inherit from '" + candidate->name + "'.",
                                        "break the inheritance cycle");
                        } else {
                            classType->superclass = candidate;
                        }
                    } else {
                        reportError(DiagnosticCode::E0007_UNKNOWN_TYPE_NAME, *(*classStmt)->superclass,
                                    "Base class '" + (*classStmt)->superclass->lexeme + "' is not defined.");
                    }
                }

                // Register fields and methods
                for (const auto& member : (*classStmt)->members) {
                    if (auto varDecl = std::get_if<std::unique_ptr<VarDeclStmt>>(&member.declaration.as)) {
                        const std::string& fieldName = (*varDecl)->name.lexeme;

                        if (classType->fields.find(fieldName) != classType->fields.end()) {
                            reportError(DiagnosticCode::E0004_DUPLICATE_DECLARATION, (*varDecl)->name,
                                        "Duplicate field '" + fieldName + "' in class '" + classType->name + "'.");
                            continue;
                        }

                        const Type* fType = resolveTypeFromToken((*varDecl)->type);
                        classType->fields[fieldName] = MemberInfo{fType, member.access};
                    } else if (auto funcDecl = std::get_if<std::unique_ptr<FunctionDeclStmt>>(&member.declaration.as)) {
                        const std::string& methodName = (*funcDecl)->name.lexeme;

                        if (classType->methods.find(methodName) != classType->methods.end()) {
                            reportError(DiagnosticCode::E0004_DUPLICATE_DECLARATION, (*funcDecl)->name,
                                        "Duplicate method '" + methodName + "' in class '" + classType->name + "'.");
                            continue;
                        }

                        const Type* retType = resolveTypeFromToken((*funcDecl)->returnType);
                        std::vector<const Type*> pTypes;
                        for (const auto& p : (*funcDecl)->parameters) {
                            pTypes.push_back(resolveTypeFromToken(p.type));
                        }
                        classType->methods[methodName] = MemberInfo{
                            Type::makeFunction(retType, pTypes),
                            member.access
                        };
                    }
                }
            }
        }
        // Register global function prototypes
        else if (auto funcStmt = std::get_if<std::unique_ptr<FunctionDeclStmt>>(&stmt.as)) {
            const Type* returnType = resolveTypeFromToken((*funcStmt)->returnType);
            std::vector<const Type*> paramTypes;
            for (const auto& p : (*funcStmt)->parameters) {
                paramTypes.push_back(resolveTypeFromToken(p.type));
            }
            const Type* funcType = Type::makeFunction(returnType, paramTypes);

            if (!symbolTable.define(Symbol((*funcStmt)->name.lexeme, funcType, SymbolKind::FUNCTION, (*funcStmt)->name))) {
                reportError(DiagnosticCode::E0004_DUPLICATE_DECLARATION, (*funcStmt)->name,
                            "Redefinition of function '" + (*funcStmt)->name.lexeme + "'.");
            }
        }
    }
}

void SemanticAnalyzer::typeCheck(const std::vector<Stmt>& ast) {
    // Sequentially execute Pass 2 type-checking and validation across all AST statements
    for (const auto& stmt : ast) {
        execute(stmt);
    }
    // Check for unused global variables after full analysis traversal
    for (const auto& [name, sym] : symbolTable.getGlobalScope()->getAllSymbols()) {
        if (sym.kind == SymbolKind::VARIABLE && !sym.isUsed && !name.empty() && name[0] != '_') {
            diagnostics.reportWarning(
                DiagnosticCode::W0001_UNUSED_VARIABLE,
                SourceSpan(sym.token),
                "Variable '" + name + "' is declared globally but never used.",
                "if this is intentional, prefix the name with an underscore: '_" + name + "'"
            );
        }
    }
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<LiteralExpr>& node) {
    // Map AST literal tokens directly to their immutable primitive Type representations
    switch (node->value.type) {
        case TokenType::INT_LITERAL:
        case TokenType::HEX_LITERAL:
        case TokenType::BIN_LITERAL:
        case TokenType::OCT_LITERAL:
            {
                long long val = node->value.asInt();
                // Verify value fits within target 32-bit signed integer boundaries
                if (val > 2147483647LL || val < -2147483648LL) {
                    reportError(DiagnosticCode::E0008_INTEGER_OVERFLOW, node->value,
                                "Integer literal '" + node->value.lexeme + "' exceeds 32-bit signed range [-2147483648, 2147483647].",
                                "reduce literal magnitude to fit in a 32-bit integer");
                    return Type::getError();
                }
                return Type::getInt();
            }
        case TokenType::FLOAT_LITERAL: return Type::getFloat();
        case TokenType::CHAR_LITERAL:  return Type::getChar();
        case TokenType::STRING_LITERAL:
        case TokenType::RAW_STRING_LITERAL: return Type::getString();
        case TokenType::KW_TRUE:
        case TokenType::KW_FALSE:      return Type::getBool();
        case TokenType::KW_NULL:       return Type::getNull();
        default:                       return Type::getError();
    }
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<VariableExpr>& node) {
    Symbol* sym = symbolTable.resolveSymbol(node->name.lexeme);
    if (sym == nullptr) {
        std::string suggestion = diagnostics.findClosestMatch(node->name.lexeme, getVisibleSymbolNames());
        std::string hint = suggestion.empty() ? "ensure variable is declared before use" : "did you mean '" + suggestion + "'?";

        reportError(DiagnosticCode::E0001_UNDECLARED_VARIABLE, node->name,
                    "Variable '" + node->name.lexeme + "' is not declared in this scope.", hint);
        return Type::getError();
    }

    // Mark variable as read/used
    sym->isUsed = true;

    // Definite assignment check: ensure variable has a value before reading
    if (!sym->isInitialized) {
        reportError(DiagnosticCode::E0002_UNINITIALIZED_VARIABLE, node->name,
                    "Variable '" + node->name.lexeme + "' is used before being initialized.",
                    "assign a value to '" + node->name.lexeme + "' before reading it");
        return Type::getError();
    }
    return sym->type;
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<ThisExpr>& node) {
    // Enforce that 'this' is strictly used within active class method scopes
    if (currentClass == nullptr) {
        reportError(DiagnosticCode::E0026_THIS_OUTSIDE_CLASS, node->keyword,
                    "Cannot use 'this' outside of a class method.",
                    "'this' refers to the instance of the enclosing class");
        return Type::getError();
    }
    return currentClass;
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<BinaryExpr>& node) {
    // 1. Assignment operator '=' (Handle left-hand side WITHOUT uninitialized read check)
    if (node->op.type == TokenType::OP_ASSIGN) {
        if (!isLValue(node->left)) {
            std::string hint = "assign to a variable or object field instead";
            if (std::holds_alternative<std::unique_ptr<LiteralExpr>>(node->left.as)) {
                hint = "did you mean to use '==' for equality comparison?";
            }
            reportError(DiagnosticCode::E0005_INVALID_ASSIGNMENT_TARGET, getExprToken(node->left),
                        "Invalid assignment target: left-hand side is not an assignable variable.", hint);
            return Type::getError();
        }

        const Type* rightType = evaluate(node->right);
        const Type* leftType = Type::getError();

        // If target is a variable, resolve its type directly and mark as initialized
        if (auto varExpr = std::get_if<std::unique_ptr<VariableExpr>>(&node->left.as)) {
            Symbol* sym = symbolTable.resolveSymbol((*varExpr)->name.lexeme);
            if (sym == nullptr) {
                reportError(DiagnosticCode::E0001_UNDECLARED_VARIABLE, (*varExpr)->name,
                            "Variable '" + (*varExpr)->name.lexeme + "' is not declared.");
                return Type::getError();
            }
            if (sym->isConstant) {
                reportError(DiagnosticCode::E0003_CANNOT_ASSIGN_CONSTANT, getExprToken(node->left),
                            "Cannot assign to constant variable '" + sym->name + "'.",
                            "variables declared with 'const' are immutable");
                return Type::getError();
            }
            // Mark variable as definitely initialized!
            sym->isInitialized = true;
            leftType = sym->type;
        } else {
            leftType = evaluate(node->left);
        }

        if (leftType->isError() || rightType->isError()) return Type::getError();

        if (!rightType->isAssignableTo(leftType)) {
            if (rightType->isNull() && !leftType->isNullable) {
                reportError(DiagnosticCode::E0013_NULL_ASSIGNED_TO_NON_NULLABLE, getExprToken(node->right),
                            "Cannot assign 'null' to non-nullable target of type '" + leftType->toString() + "'.",
                            "declare target as nullable with '" + leftType->toString() + "?'");
            } else {
                reportError(DiagnosticCode::E0006_TYPE_MISMATCH, getExprToken(node->right),
                            "Cannot assign value of type '" + rightType->toString() +
                            "' to target of type '" + leftType->toString() + "'.",
                            "types must match or be implicitly convertible");
            }
            return Type::getError();
        }
        return leftType;
    }

    // Normal bottom-up evaluation for all other binary operators
    const Type* leftType = evaluate(node->left);
    const Type* rightType = evaluate(node->right);
    if (leftType->isError() || rightType->isError()) return Type::getError();

    // Compound assignment operators (+=, -=, *=, /=, %=)
    if (node->op.type == TokenType::OP_PLUS_ASS || node->op.type == TokenType::OP_MIN_ASS ||
        node->op.type == TokenType::OP_MUL_ASS || node->op.type == TokenType::OP_DIV_ASS ||
        node->op.type == TokenType::OP_MOD_ASS) {
        if (!isLValue(node->left)) {
            reportError(DiagnosticCode::E0005_INVALID_ASSIGNMENT_TARGET, getExprToken(node->left),
                        "Left-hand side of compound assignment is not assignable.");
            return Type::getError();
        }
        // Allow '+=' on strings for in-place concatenation
        if (node->op.type == TokenType::OP_PLUS_ASS && (leftType->isString() || rightType->isString())) {
            return leftType;
        }
        if (!leftType->isNumeric() || !rightType->isNumeric()) {
            reportError(DiagnosticCode::E0010_INVALID_OPERATOR_OPERANDS, node->op,
                        "Compound operator requires numeric operands.",
                        "both sides must be 'int' or 'float'");
            return Type::getError();
        }
        return leftType;
    }

    // Addition and string concatenation
    if (node->op.type == TokenType::OP_PLUS) {
        if (leftType->isString() || rightType->isString()) {
            return Type::getString();
        }
        if (leftType->isInteger() && rightType->isInteger()) return Type::getInt();
        if (leftType->isNumeric() && rightType->isNumeric()) return Type::getFloat();

        reportError(DiagnosticCode::E0010_INVALID_OPERATOR_OPERANDS, node->op,
                    "Operator '+' cannot be applied between '" + leftType->toString() +
                    "' and '" + rightType->toString() + "'.",
                    "'+' is only supported for numeric addition and string concatenation");
        return Type::getError();
    }

    // Arithmetic operators (-, *, /, %)
    if (node->op.type == TokenType::OP_MINUS || node->op.type == TokenType::OP_STAR ||
        node->op.type == TokenType::OP_SLASH || node->op.type == TokenType::OP_MOD) {
        if (leftType->isInteger() && rightType->isInteger()) return Type::getInt();
        if (leftType->isNumeric() && rightType->isNumeric()) return Type::getFloat();

        std::string hint = "strings do not support arithmetic; use '+' for concatenation";
        reportError(DiagnosticCode::E0010_INVALID_OPERATOR_OPERANDS, node->op,
                    "Arithmetic operator requires numeric operands.", hint);
        return Type::getError();
    }

    // Equality comparisons (==, !=)
    if (node->op.type == TokenType::OP_EQ_EQ || node->op.type == TokenType::OP_NOT_EQ) {
        if (!leftType->isAssignableTo(rightType) && !rightType->isAssignableTo(leftType)) {
            reportError(DiagnosticCode::E0006_TYPE_MISMATCH, node->op,
                        "Cannot compare incompatible types '" + leftType->toString() +
                        "' and '" + rightType->toString() + "'.",
                        "ensure both operands have compatible types");
            return Type::getError();
        }
        return Type::getBool();
    }

    // Relational comparisons (<, <=, >, >=)
    if (node->op.type == TokenType::OP_LESS || node->op.type == TokenType::OP_LESS_EQ ||
        node->op.type == TokenType::OP_GRT || node->op.type == TokenType::OP_GRT_EQ) {
        if (!leftType->isNumeric() || !rightType->isNumeric()) {
            reportError(DiagnosticCode::E0010_INVALID_OPERATOR_OPERANDS, node->op,
                        "Relational comparisons require numeric operands.",
                        "operands must be 'int' or 'float'");
            return Type::getError();
        }
        return Type::getBool();
    }

    // Logical boolean operators (&&, ||)
    if (node->op.type == TokenType::OP_AND || node->op.type == TokenType::OP_OR) {
        if (!leftType->isBool() || !rightType->isBool()) {
            reportError(DiagnosticCode::E0009_NON_BOOLEAN_CONDITION, node->op,
                        "Logical operators require boolean operands.",
                        "use comparison expressions producing 'bool'");
            return Type::getError();
        }
        return Type::getBool();
    }
    // Handle type-testing operator 'is' (e.g. hero is Character)
    if (node->op.type == TokenType::KW_IS) {
        // Validate that types share an inheritance or identity relationship
        if (!leftType->isAssignableTo(rightType) && !rightType->isAssignableTo(leftType)) {
            reportError(DiagnosticCode::E0006_TYPE_MISMATCH, node->op,
                        "Type check 'is' cannot compare incompatible types '" +
                        leftType->toString() + "' and '" + rightType->toString() + "'.");
            return Type::getError();
        }
        return Type::getBool();
    }

    return Type::getError();
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<RangeExpr>& node) {
    const Type* startType = evaluate(node->start);
    const Type* endType = evaluate(node->end);

    // Range bounds must both be integer expressions
    if (!startType->isInteger() || !endType->isInteger()) {
        reportError(DiagnosticCode::E0012_INVALID_RANGE_BOUNDS, node->op,
                    "Range bounds must evaluate to integers.",
                    "e.g. '0..10'");
        return Type::getError();
    }
    return Type::makeArray(Type::getInt());
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<UnaryExpr>& node) {
    const Type* operandType = evaluate(node->operand);
    if (operandType->isError()) return Type::getError();

    // Logical negation requires boolean operand
    if (node->op.type == TokenType::OP_NOT) {
        if (!operandType->isBool()) {
            reportError(DiagnosticCode::E0009_NON_BOOLEAN_CONDITION, node->op,
                        "Logical NOT '!' requires a boolean operand.",
                        "for bitwise inversion use '~'");
            return Type::getError();
        }
        return Type::getBool();
    }

    // Unary sign operators require numeric operand
    if (node->op.type == TokenType::OP_MINUS || node->op.type == TokenType::OP_PLUS) {
        if (!operandType->isNumeric()) {
            reportError(DiagnosticCode::E0010_INVALID_OPERATOR_OPERANDS, node->op,
                        "Unary operator requires a numeric operand.");
            return Type::getError();
        }
        return operandType;
    }

    // Increment and decrement require numeric assignable target
    if (node->op.type == TokenType::OP_INC || node->op.type == TokenType::OP_DEC) {
        if (!isLValue(node->operand)) {
            reportError(DiagnosticCode::E0005_INVALID_ASSIGNMENT_TARGET, getExprToken(node->operand),
                        "Target of increment/decrement must be an assignable variable.",
                        "increment/decrement modifies memory and cannot be applied to literals");
            return Type::getError();
        }
        if (!operandType->isNumeric()) {
            reportError(DiagnosticCode::E0010_INVALID_OPERATOR_OPERANDS, node->op,
                        "Increment/decrement requires a numeric operand.");
            return Type::getError();
        }
        return operandType;
    }

    return Type::getError();
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<GroupingExpr>& node) {
    return evaluate(node->expression);
}


const Type* SemanticAnalyzer::operator()(const std::unique_ptr<CallExpr>& node) {
    const Type* calleeType = evaluate(node->callee);
    if (calleeType->isError()) return Type::getError();

    // Instantiating a struct or class acts as a constructor returning the type instance
    if (calleeType->kind == TypeKind::CLASS || calleeType->kind == TypeKind::STRUCT) {
        return calleeType;
    }

    // Validate standard function invocation
    if (calleeType->kind == TypeKind::FUNCTION) {
        size_t totalParams = calleeType->paramTypes.size();
        size_t passedArgs = node->arguments.size();

        // Enforce maximum parameter capacity
        if (passedArgs > totalParams) {
            reportError(DiagnosticCode::E0032_ARGUMENT_COUNT_MISMATCH, node->paren,
                        "Function expects at most " + std::to_string(totalParams) +
                        " arguments, but received " + std::to_string(passedArgs) + ".",
                        "remove extraneous arguments");
            return Type::getError();
        }

        // Validate argument type assignability
        for (size_t i = 0; i < passedArgs; ++i) {
            const Type* argType = evaluate(node->arguments[i]);
            const Type* paramType = calleeType->paramTypes[i];
            if (!argType->isAssignableTo(paramType)) {
                reportError(DiagnosticCode::E0033_ARGUMENT_TYPE_MISMATCH, getExprToken(node->arguments[i]),
                            "Argument " + std::to_string(i + 1) + " of type '" +
                            argType->toString() + "' is not assignable to parameter of type '" +
                            paramType->toString() + "'.",
                            "pass a compatible expression matching parameter type");
            }
        }
        return calleeType->returnType;
    }

    reportError(DiagnosticCode::E0031_NOT_CALLABLE, node->paren,
                "Target expression is not callable as a function.",
                "only functions, classes, and structs can be invoked with '()'");
    return Type::getError();
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<MemberAccessExpr>& node) {
    const Type* baseType = evaluate(node->accessed);
    if (baseType->isError()) return Type::getError();
    // Prevent direct member access on nullable instances
    if (baseType->isNullable) {
        reportError(DiagnosticCode::E0014_NULLABLE_MEMBER_DEREFERENCE, node->dot,
                    "Cannot access member '" + node->member.lexeme + "' on nullable type '" + baseType->name + "'.",
                    "check for null using 'if (" + getExprToken(node->accessed).lexeme + " != null):' before accessing members");
        return Type::getError();
    }
    if (baseType->kind != TypeKind::CLASS && baseType->kind != TypeKind::STRUCT) {
        reportError(DiagnosticCode::E0030_MEMBER_NOT_FOUND, node->dot,
                    "Cannot access member '" + node->member.lexeme +
                    "' on non-class/non-struct type '" + baseType->toString() + "'.",
                    "primitive types do not have fields or methods");
        return Type::getError();
    }

    // Access control validation lambda
    auto checkAccess = [&](const MemberInfo& info, const Type* declaringClass) -> bool {
        if (info.access == AccessModifier::PUBLIC) return true;
        if (info.access == AccessModifier::PRIVATE) return currentClass == declaringClass;
        if (info.access == AccessModifier::PROTECTED) {
            if (currentClass == declaringClass) return true;
            if (currentClass != nullptr && currentClass->isAssignableTo(declaringClass)) return true;
            return false;
        }
        return false;
    };

// Look up fields on the base class
    auto fieldIt = baseType->fields.find(node->member.lexeme);
    if (fieldIt != baseType->fields.end()) {
        if (!checkAccess(fieldIt->second, baseType)) {
            DiagnosticCode errCode = (fieldIt->second.access == AccessModifier::PROTECTED)
                                         ? DiagnosticCode::E0029_PROTECTED_MEMBER_ACCESS
                                         : DiagnosticCode::E0028_PRIVATE_MEMBER_ACCESS;
            std::string accessStr = (fieldIt->second.access == AccessModifier::PROTECTED) ? "protected" : "private";
            reportError(errCode, node->member,
                        "Cannot access '" + accessStr + "' field '" + node->member.lexeme + "' of class '" + baseType->name + "'.",
                        accessStr == "private" ? "consider making '" + node->member.lexeme + "' public or providing a getter method"
                                               : "protected members are only accessible within the class and its subclasses");
        }
        return fieldIt->second.type;
    }

    // Look up methods on the base class
    auto methodIt = baseType->methods.find(node->member.lexeme);
    if (methodIt != baseType->methods.end()) {
        if (!checkAccess(methodIt->second, baseType)) {
            DiagnosticCode errCode = (methodIt->second.access == AccessModifier::PROTECTED)
                                         ? DiagnosticCode::E0029_PROTECTED_MEMBER_ACCESS
                                         : DiagnosticCode::E0028_PRIVATE_MEMBER_ACCESS;
            std::string accessStr = (methodIt->second.access == AccessModifier::PROTECTED) ? "protected" : "private";
            reportError(errCode, node->member,
                        "Cannot access '" + accessStr + "' method '" + node->member.lexeme + "' of class '" + baseType->name + "'.",
                        accessStr == "private" ? "private methods can only be called from inside the class"
                                               : "protected methods are only accessible within the class and its subclasses");
        }
        return methodIt->second.type;
    }

    // Look up members in the superclass inheritance chain
    const Type* parent = baseType->superclass;
    while (parent != nullptr) {
        auto pField = parent->fields.find(node->member.lexeme);
        if (pField != parent->fields.end()) {
            if (!checkAccess(pField->second, parent)) {
                DiagnosticCode errCode = (pField->second.access == AccessModifier::PROTECTED)
                                             ? DiagnosticCode::E0029_PROTECTED_MEMBER_ACCESS
                                             : DiagnosticCode::E0028_PRIVATE_MEMBER_ACCESS;
                std::string accessStr = (pField->second.access == AccessModifier::PROTECTED) ? "protected" : "private";
                reportError(errCode, node->member,
                            "Cannot access '" + accessStr + "' member of base class '" + parent->name + "'.");
            }
            return pField->second.type;
        }
        auto pMethod = parent->methods.find(node->member.lexeme);
        if (pMethod != parent->methods.end()) {
            if (!checkAccess(pMethod->second, parent)) {
                DiagnosticCode errCode = (pMethod->second.access == AccessModifier::PROTECTED)
                                             ? DiagnosticCode::E0029_PROTECTED_MEMBER_ACCESS
                                             : DiagnosticCode::E0028_PRIVATE_MEMBER_ACCESS;
                std::string accessStr = (pMethod->second.access == AccessModifier::PROTECTED) ? "protected" : "private";
                reportError(errCode, node->member,
                            "Cannot access '" + accessStr + "' method of base class '" + parent->name + "'.");
            }
            return pMethod->second.type;
        }
        parent = parent->superclass;
    }

    // Suggest matching member on typo
    std::vector<std::string> memberNames;
    for (const auto& [fName, _] : baseType->fields) memberNames.push_back(fName);
    for (const auto& [mName, _] : baseType->methods) memberNames.push_back(mName);
    std::string hint = diagnostics.findClosestMatch(node->member.lexeme, memberNames);
    if (!hint.empty()) hint = "did you mean '" + hint + "'?";

    reportError(DiagnosticCode::E0030_MEMBER_NOT_FOUND, node->member,
                "Type '" + baseType->name + "' has no member named '" + node->member.lexeme + "'.", hint);
    return Type::getError();
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<ArrayAccessExpr>& node) {
    const Type* arrayType = evaluate(node->array);
    const Type* indexType = evaluate(node->index);

    if (!indexType->isInteger()) {
        reportError(DiagnosticCode::E0018_INVALID_SUBSCRIPT_INDEX, getExprToken(node->index),
                    "Array index must evaluate to an integer.");
    }

    // Prevent direct indexing on nullable arrays (e.g. int[]? list)
    if (arrayType->isNullable) {
        reportError(DiagnosticCode::E0015_NULLABLE_ARRAY_SUBSCRIPT, node->openingBracket,
                    "Cannot apply subscript operator '[]' on nullable array type '" + arrayType->name + "'.",
                    "check for null using 'if (" + getExprToken(node->array).lexeme + " != null):' before indexing");
        return Type::getError();
    }

    if (arrayType->kind != TypeKind::ARRAY) {
        reportError(DiagnosticCode::E0017_INVALID_SUBSCRIPT_TARGET, getExprToken(node->array),
                    "Subscript operator '[]' can only be applied to array types.");
        return Type::getError();
    }

    return arrayType->elementType;
}
const Type* SemanticAnalyzer::operator()(const std::unique_ptr<ArrayLiteralExpr>& node) {
    if (node->elements.empty()) {
        return Type::makeArray(Type::getVoid());
    }

    const Type* commonType = nullptr;

    for (const auto& elem : node->elements) {
        const Type* elemType = evaluate(elem);
        if (elemType->isError()) return Type::getError();

        if (commonType == nullptr) {
            commonType = elemType;
            continue;
        }

        // Direct canonical pointer match or subtype assignability
        if (elemType == commonType || elemType->isAssignableTo(commonType)) {
            continue;
        }

        // Promote common type if new element is a wider supertype
        if (commonType->isAssignableTo(elemType)) {
            commonType = elemType;
            continue;
        }

        // Unify concrete types with null to infer nullable array (T + null -> T?)
        if (elemType->isNull()) {
            if (!commonType->isNullable && !commonType->isNull()) {
                commonType = Type::makeNullable(commonType);
            }
            continue;
        }
        if (commonType->isNull()) {
            commonType = Type::makeNullable(elemType);
            continue;
        }

        reportError(DiagnosticCode::E0016_ARRAY_ELEMENT_TYPE_MISMATCH, getExprToken(elem),
                    "Array element of type '" + elemType->toString() +
                    "' is incompatible with inferred array type '" + commonType->toString() + "'.",
                    "all elements in an array literal must share a compatible base type");
        return Type::getError();
    }

    return Type::makeArray(commonType);
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<TernaryExpr>& node) {
    const Type* condType = evaluate(node->condition);
    if (!condType->isBool()) {
        reportError(DiagnosticCode::E0009_NON_BOOLEAN_CONDITION, node->questionMark,
                    "Ternary condition must evaluate to a boolean expression.",
                    "ensure the condition expression evaluates to 'bool'");
    }

    const Type* trueType = evaluate(node->trueBranch);
    const Type* falseType = evaluate(node->falseBranch);

    if (!falseType->isAssignableTo(trueType) && !trueType->isAssignableTo(falseType)) {
        reportError(DiagnosticCode::E0011_INCOMPATIBLE_TERNARY_BRANCHES, node->questionMark,
                    "Ternary branch types are incompatible ('" +
                    trueType->toString() + "' vs '" + falseType->toString() + "').",
                    "both branches must yield compatible return types");
        return Type::getError();
    }

    return trueType;
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<CastExpr>& node) {
    const Type* exprType = evaluate(node->expression);
    const Type* targetType = resolveTypeFromToken(node->targetType);

    if (exprType->isError() || targetType->isError()) return Type::getError();

    // 1. Identity cast (T as T)
    if (exprType == targetType) return targetType;

    // 2. Numeric casting (int as float, float as int)
    if (exprType->isNumeric() && targetType->isNumeric()) {
        return targetType;
    }

    // 3. Downcasting and Upcasting in OOP hierarchy
    if (exprType->kind == TypeKind::CLASS && targetType->kind == TypeKind::CLASS) {
        if (exprType->isAssignableTo(targetType) || targetType->isAssignableTo(exprType)) {
            return targetType;
        }
    }

    reportError(DiagnosticCode::E0006_TYPE_MISMATCH, node->asToken,
                "Cannot cast expression of type '" + exprType->toString() +
                "' to incompatible target type '" + targetType->toString() + "'.");
    return Type::getError();
}

void SemanticAnalyzer::operator()(const std::unique_ptr<ExpressionStmt>& node) {
    evaluate(node->expression);
}

void SemanticAnalyzer::operator()(const std::unique_ptr<VarDeclStmt>& node) {
    const Type* declaredType = resolveTypeFromToken(node->type);
    // Enforce that variables cannot be declared with 'void' type
    const Type* checkVoidType = declaredType;
    //Check of original type of the array
    while (checkVoidType->kind == TypeKind::ARRAY) {
        checkVoidType = checkVoidType->elementType;
    }
    if (checkVoidType->isVoid()) {
        reportError(DiagnosticCode::E0041_VOID_VARIABLE_DECLARATION, node->name,
                    "Variable '" + node->name.lexeme + "' cannot be declared of type '" + declaredType->toString() + "'.",
                    "variables must have a concrete, non-void data type");
        return;
    }

    auto existingSym = symbolTable.resolve(node->name.lexeme);
    if (existingSym.has_value() && symbolTable.existsInCurrentScope(node->name.lexeme)) {
        // Report multi-span connecting original declaration and invalid redeclaration
        diagnostics.reportMultiSpan(
            DiagnosticCode::E0004_DUPLICATE_DECLARATION,
            "Variable '" + node->name.lexeme + "' is already defined in this block.",
            {
                SpanLabel{SourceSpan(existingSym->token), "first declared here", false},
                SpanLabel{SourceSpan(node->name), "re-declared here", true}
            },
            "choose a distinct identifier or remove the duplicate declaration"
        );
    }
    // Determine initialization status and validate expression assignability
    bool hasInit = (node->initializer != nullptr);
    if (hasInit) {
        const Type* initType = evaluate(*node->initializer);
        if (!initType->isAssignableTo(declaredType)) {
            if (initType->isNull() && !declaredType->isNullable) {
                reportError(DiagnosticCode::E0013_NULL_ASSIGNED_TO_NON_NULLABLE, getExprToken(*node->initializer),
                            "Cannot assign 'null' to non-nullable variable '" + node->name.lexeme + "' of type '" + declaredType->toString() + "'.",
                            "declare variable as nullable with '" + declaredType->toString() + "?'");
            } else {
                std::string hint = "ensure expression type matches declared variable type";
                if (declaredType->isString()) {
                    hint = "consider using a format string f\"...\" to convert values to string";
                } else if (declaredType->isInteger() && initType->isFloat()) {
                    hint = "floats cannot be implicitly narrowed to int; use explicit truncation";
                }
                reportError(DiagnosticCode::E0006_TYPE_MISMATCH, getExprToken(*node->initializer),
                            "Cannot initialize variable of type '" + declaredType->toString() +
                            "' with expression of type '" + initType->toString() + "'.", hint);
            }
        }
    }



    // Define symbol with accurate initialization and constness state
    Symbol varSym(node->name.lexeme, declaredType, SymbolKind::VARIABLE, node->name);
    varSym.isInitialized = hasInit;
    varSym.isConstant = node->isConst; // Propagate 'const' declaration!
    varSym.isUsed = false; // Initially unused

    if (!symbolTable.define(varSym)) {
        reportError(DiagnosticCode::E0004_DUPLICATE_DECLARATION, node->name,
                    "Variable '" + node->name.lexeme + "' is already defined in this block.",
                    "choose a distinct identifier or remove duplicate declaration");
    }
}

void SemanticAnalyzer::operator()(const std::unique_ptr<BlockStmt>& node) {
    symbolTable.enterScope(ScopeKind::BLOCK);
    for (const auto& stmt : node->statements) {
        execute(stmt);
    }

    // Check for unused local variables and emit non-breaking warnings
    for (const auto& [name, sym] : symbolTable.getCurrentScope()->getAllSymbols()) {
        if (sym.kind == SymbolKind::VARIABLE && !sym.isUsed && !name.empty() && name[0] != '_') {
            diagnostics.reportWarning(
                DiagnosticCode::W0001_UNUSED_VARIABLE,
                SourceSpan(sym.token),
                "Variable '" + name + "' is declared but its value is never read.",
                "if this is intentional, prefix the name with an underscore: '_" + name + "'"
            );
        }
    }

    symbolTable.exitScope();
}

void SemanticAnalyzer::operator()(const std::unique_ptr<IfStmt>& node) {
    // 1. Evaluate the condition expression and ensure it produces a boolean value
    const Type* condType = evaluate(node->condition);
    if (!condType->isBool()) {
        reportError(DiagnosticCode::E0009_NON_BOOLEAN_CONDITION, node->keyword,
                    "If condition must be a boolean expression.",
                    "consider comparing with zero or null: '(condition != 0)'");
    }

    // 2. Recursive helper to extract variable names guaranteed non-null when the condition is true.
    // If the condition is compound (e.g. 'a != null && b != null'), both operands must evaluate to true
    // for the 'then' block to execute, so we recursively inspect both branches of logical AND ('&&').
    std::function<void(const Expr&, std::vector<std::string>&)> collectNonNullVars;
    collectNonNullVars = [&](const Expr& expr, std::vector<std::string>& outVars) {
        const Expr* innerExpr = &expr;

        // Strip any nested parentheses (GroupingExpr) to reach the underlying comparison
        while (auto group = std::get_if<std::unique_ptr<GroupingExpr>>(&innerExpr->as)) {
            innerExpr = &(*group)->expression;
        }

        if (auto bin = std::get_if<std::unique_ptr<BinaryExpr>>(&innerExpr->as)) {
            // Logical AND: both sides must be true, so collect non-null variables from both operands
            if ((*bin)->op.type == TokenType::OP_AND) {
                collectNonNullVars((*bin)->left, outVars);
                collectNonNullVars((*bin)->right, outVars);
                return;
            }

            // Check for inequality against null literal: either 'var != null' or 'null != var'
            if ((*bin)->op.type == TokenType::OP_NOT_EQ) {
                // Case A: variable != null
                if (auto varNode = std::get_if<std::unique_ptr<VariableExpr>>(&(*bin)->left.as)) {
                    if (auto litNode = std::get_if<std::unique_ptr<LiteralExpr>>(&(*bin)->right.as)) {
                        if ((*litNode)->value.type == TokenType::KW_NULL) {
                            outVars.push_back((*varNode)->name.lexeme);
                        }
                    }
                }
                // Case B: null != variable
                else if (auto litNode = std::get_if<std::unique_ptr<LiteralExpr>>(&(*bin)->left.as)) {
                    if (auto varNode = std::get_if<std::unique_ptr<VariableExpr>>(&(*bin)->right.as)) {
                        if ((*litNode)->value.type == TokenType::KW_NULL) {
                            outVars.push_back((*varNode)->name.lexeme);
                        }
                    }
                }
            }
        }
    };

    // 3. Helper to temporarily promote nullable variables to non-nullable within the active scope.
    // We define a new Symbol in the current block scope with the base type (stripping '?').
    // This locally shadows the outer nullable variable; upon exiting the scope, it reverts automatically.
    auto applySmartCasts = [&](const std::vector<std::string>& varNames) {
        for (const auto& varName : varNames) {
            auto symOpt = symbolTable.resolve(varName);
            if (symOpt.has_value() && symOpt->type->isNullable) {
                // Strip trailing '?' suffix to resolve the underlying concrete type (e.g. "Dog?" -> "Dog")
                std::string nonNullName = symOpt->type->name;
                if (!nonNullName.empty() && nonNullName.back() == '?') {
                    nonNullName.pop_back();
                }

                // Resolve the non-nullable type pointer from the cleaned name
                Token baseTok = symOpt->token;
                baseTok.lexeme = nonNullName;
                const Type* nonNullType = resolveTypeFromToken(baseTok);

                // Insert the promoted non-nullable symbol into the current scope (local shadowing)
                Symbol promotedSym(varName, nonNullType, SymbolKind::VARIABLE, symOpt->token);
                promotedSym.isInitialized = true;
                promotedSym.isUsed = true;
                symbolTable.define(promotedSym);
            }
        }
    };

    // --- PHASE 1: ANALYZE THEN BRANCH ---
    // Extract all variables verified non-null by the root if-condition
    std::vector<std::string> thenPromotions;
    collectNonNullVars(node->condition, thenPromotions);

    // Open an isolated block scope for the 'then' body
    symbolTable.enterScope(ScopeKind::BLOCK);
    // Apply local non-nullable promotions (shadowing outer nullable symbols)
    applySmartCasts(thenPromotions);
    // Type-check all statements inside the 'then' block
    execute(node->thenBranch);
    // Close the scope; all promoted shadow symbols are discarded, restoring nullable types outside
    symbolTable.exitScope();

    // --- PHASE 2: ANALYZE ELIF BRANCHES ---
    for (const auto& elifBranch : node->elifBranches) {
        // Validate that each elif condition is boolean
        const Type* elifCondType = evaluate(elifBranch.condition);
        if (!elifCondType->isBool()) {
            reportError(DiagnosticCode::E0009_NON_BOOLEAN_CONDITION, elifBranch.keyword, "Elif condition must be a boolean expression.");
        }

        // Collect non-null variables specific to this elif condition
        std::vector<std::string> elifPromotions;
        collectNonNullVars(elifBranch.condition, elifPromotions);

        // Open an isolated block scope for this specific elif branch
        symbolTable.enterScope(ScopeKind::BLOCK);
        applySmartCasts(elifPromotions);
        execute(elifBranch.block);
        symbolTable.exitScope();
    }

    // --- PHASE 3: ANALYZE ELSE BRANCH ---
    if (node->elseBranch != nullptr) {
        std::vector<std::string> elsePromotions;

        // Inverted check: if condition was explicitly 'var == null', then in the 'else' branch
        // we are guaranteed that 'var' is NOT null, so we can safely promote it!
        const Expr* elseConditionExpr = &node->condition;
        while (auto group = std::get_if<std::unique_ptr<GroupingExpr>>(&elseConditionExpr->as)) {
            elseConditionExpr = &(*group)->expression;
        }
        if (auto bin = std::get_if<std::unique_ptr<BinaryExpr>>(&elseConditionExpr->as)) {
            if ((*bin)->op.type == TokenType::OP_EQ_EQ) {
                if (auto varNode = std::get_if<std::unique_ptr<VariableExpr>>(&(*bin)->left.as)) {
                    if (auto litNode = std::get_if<std::unique_ptr<LiteralExpr>>(&(*bin)->right.as)) {
                        if ((*litNode)->value.type == TokenType::KW_NULL) {
                            elsePromotions.push_back((*varNode)->name.lexeme);
                        }
                    }
                }
            }
        }

        // Open an isolated block scope for the 'else' body
        symbolTable.enterScope(ScopeKind::BLOCK);
        applySmartCasts(elsePromotions);
        execute(*node->elseBranch);
        symbolTable.exitScope();
    }
}

void SemanticAnalyzer::operator()(const std::unique_ptr<WhileStmt>& node) {
    const Type* condType = evaluate(node->condition);
    if (!condType->isBool()) {
        reportError(DiagnosticCode::E0009_NON_BOOLEAN_CONDITION, node->keyword,
                    "While loop condition must be a boolean expression.",
                    "consider comparing explicitly: '(condition != 0)'");
    }

    loopDepth++;
    execute(node->body);
    loopDepth--;
}

void SemanticAnalyzer::operator()(const std::unique_ptr<ForStmt>& node) {
    const Type* iterableType = evaluate(node->iterable);
    if (iterableType->kind != TypeKind::ARRAY) {
        reportError(DiagnosticCode::E0010_INVALID_OPERATOR_OPERANDS, node->keyword,
                    "For loop iterable must evaluate to an array or range.",
                    "use a range expression (e.g. '0..10') or an array variable");
    }

    loopDepth++;
    symbolTable.enterScope(ScopeKind::BLOCK);

    // Bind iterator variable in local loop scope
    const Type* elemType = iterableType->elementType ? iterableType->elementType : Type::getInt();
    symbolTable.define(Symbol(node->iteratorVar.lexeme, elemType, SymbolKind::VARIABLE, node->iteratorVar));

    execute(node->body);

    symbolTable.exitScope();
    loopDepth--;
}

void SemanticAnalyzer::operator()(const std::unique_ptr<FunctionDeclStmt>& node) {
    const Type* returnType = resolveTypeFromToken(node->returnType);
    currentFunctionReturnType = returnType;

    symbolTable.enterScope(ScopeKind::FUNCTION);

    bool seenDefault = false;
    for (const auto& param : node->parameters) {
        const Type* paramType = resolveTypeFromToken(param.type);

        if (param.defaultValue != nullptr) {
            seenDefault = true;
            const Type* defType = evaluate(*param.defaultValue);
            if (!defType->isAssignableTo(paramType)) {
                reportError(DiagnosticCode::E0006_TYPE_MISMATCH, getExprToken(*param.defaultValue),
                            "Default value of type '" + defType->toString() +
                            "' does not match parameter type '" + paramType->toString() + "'.",
                            "provide a default value matching the parameter type");
            }
        } else if (seenDefault) {
            reportError(DiagnosticCode::E0034_DEFAULT_PARAM_ORDER, param.name,
                        "Positional parameter '" + param.name.lexeme +
                        "' cannot follow default parameters.",
                        "place all parameters with default values at the end of the list");
        }

        symbolTable.define(Symbol(param.name.lexeme, paramType, SymbolKind::PARAMETER, param.name));
    }

    execute(node->body);

    // FLOW ANALYSIS: Verify that non-void functions guarantee a return on all execution paths
    if (!returnType->isVoid()) {
        if (!hasGuaranteedReturn(node->body)) {
            reportError(
                DiagnosticCode::E0025_NOT_ALL_PATHS_RETURN,
                node->name,
                "Function '" + node->name.lexeme + "' does not guarantee a return value on all code paths.",
                "ensure all branches (if, elif, else) return a value or add a final 'return' statement"
            );
        }
    }

    // Check for unused local variables and parameters inside function scope before exiting
    for (const auto& [name, sym] : symbolTable.getCurrentScope()->getAllSymbols()) {
        if (!sym.isUsed && !name.empty() && name[0] != '_') {
            DiagnosticCode code = (sym.kind == SymbolKind::PARAMETER)
                                    ? DiagnosticCode::W0002_UNUSED_PARAMETER
                                    : DiagnosticCode::W0001_UNUSED_VARIABLE;
            std::string entityStr = (sym.kind == SymbolKind::PARAMETER) ? "Parameter '" : "Local variable '";

            diagnostics.reportWarning(
                code,
                SourceSpan(sym.token),
                entityStr + name + "' is defined in function '" + node->name.lexeme + "' but never used.",
                "if this is intentional, prefix with an underscore: '_" + name + "'"
            );
        }
    }
    symbolTable.exitScope();
    currentFunctionReturnType = nullptr;
}

void SemanticAnalyzer::operator()(const std::unique_ptr<StructDeclStmt>& node) {
    // Declarations and field layouts were pre-populated during Pass 1
}

void SemanticAnalyzer::operator()(const std::unique_ptr<ClassDeclStmt>& node) {
    auto symOpt = symbolTable.resolve(node->name.lexeme);
    if (!symOpt.has_value()) return;

    currentClass = symOpt->type;
    symbolTable.enterScope(ScopeKind::CLASS);

    // Type-check method bodies within the class scope context
    for (const auto& member : node->members) {
        execute(member.declaration);
    }

    symbolTable.exitScope();
    currentClass = nullptr;
}

void SemanticAnalyzer::operator()(const std::unique_ptr<ReturnStmt>& node) {
    if (currentFunctionReturnType == nullptr) {
        reportError(DiagnosticCode::E0021_RETURN_OUTSIDE_FUNCTION, node->keyword,
                    "'return' statement not allowed outside of a function.",
                    "remove 'return' from global scope");
        return;
    }

    if (currentFunctionReturnType->isVoid()) {
        if (node->value != nullptr) {
            reportError(DiagnosticCode::E0024_RETURN_IN_VOID_FUNCTION, getExprToken(*node->value),
                        "Cannot return a value from a function with 'void' return type.",
                        "use 'return' without a value or change function signature");
        }
    } else {
        if (node->value == nullptr) {
            reportError(DiagnosticCode::E0023_MISSING_RETURN_VALUE, node->keyword,
                        "Missing return value: function expects return type '" +
                        currentFunctionReturnType->toString() + "'.",
                        "provide an expression matching the return type");
        } else {
            const Type* valType = evaluate(*node->value);
            if (!valType->isAssignableTo(currentFunctionReturnType)) {
                reportError(DiagnosticCode::E0022_RETURN_TYPE_MISMATCH, getExprToken(*node->value),
                        "Return value of type '" + valType->toString() +
                        "' does not match expected function return type '" +
                        currentFunctionReturnType->toString() + "'.",
                        "return an expression compatible with '" + currentFunctionReturnType->toString() + "'");
            }
        }
    }
}

void SemanticAnalyzer::operator()(const std::unique_ptr<BreakStmt>& node) {
    if (loopDepth == 0) {
        reportError(DiagnosticCode::E0019_BREAK_OUTSIDE_LOOP, node->keyword,
                    "'break' statement is only allowed inside loops.",
                    "'break' can only terminate 'while' or 'for' loop bodies");
    }
}

void SemanticAnalyzer::operator()(const std::unique_ptr<ContinueStmt>& node) {
    if (loopDepth == 0) {
        reportError(DiagnosticCode::E0020_CONTINUE_OUTSIDE_LOOP, node->keyword,
                    "'continue' statement is only allowed inside loops.",
                    "'continue' can only advance 'while' or 'for' loop iterations");
    }
}
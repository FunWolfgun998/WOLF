#include "../../include/Semantic/SemanticAnalyzer.h"
#include <iostream>

void SemanticAnalyzer::reportError(ErrorCode code, const Token& token, const std::string& message, const std::string& hint) {
    // Record diagnostic entry into engine
    diagnostics.report(code, token, message, hint);
}

std::vector<std::string> SemanticAnalyzer::getVisibleSymbolNames() const {
    std::vector<std::string> names;
    auto scope = symbolTable.getCurrentScope();

    // Traverse active lexical scope chain upwards to gather all visible identifiers
    while (scope != nullptr) {
        for (const auto& [name, sym] : scope->getAllSymbols()) {
            names.push_back(name);
        }
        scope = scope->getParent();
    }
    return names;
}

const Type* SemanticAnalyzer::resolveTypeFromToken(const Token& typeToken) {
    std::string typeName = typeToken.lexeme;

    // Built-in primitive types
    if (typeName == "int") return Type::getInt();
    if (typeName == "float") return Type::getFloat();
    if (typeName == "char") return Type::getChar();
    if (typeName == "string") return Type::getString();
    if (typeName == "bool") return Type::getBool();
    if (typeName == "void") return Type::getVoid();

    // Recursively resolve element type for array types (e.g. "int[]", "Dog[][]")
    if (typeName.size() > 2 && typeName.substr(typeName.size() - 2) == "[]") {
        Token baseToken = typeToken;
        baseToken.lexeme = typeName.substr(0, typeName.size() - 2);
        const Type* baseType = resolveTypeFromToken(baseToken);
        return Type::makeArray(baseType);
    }

    // Look up user-defined types (classes and structs) in the symbol table
    auto symbolOpt = symbolTable.resolve(typeName);
    if (symbolOpt.has_value()) {
        if (symbolOpt->kind == SymbolKind::CLASS || symbolOpt->kind == SymbolKind::STRUCT) {
            return symbolOpt->type;
        }
        reportError(ErrorCode::E0024_UNKNOWN_TYPE_NAME, typeToken, "'" + typeName + "' is a variable name, not a type.");
        return Type::getError();
    }

    // Suggest closest matching identifier on typo
    std::string suggestion = diagnostics.findClosestMatch(typeName, getVisibleSymbolNames());
    std::string hint = suggestion.empty() ? "" : "did you mean '" + suggestion + "'?";
    reportError(ErrorCode::E0024_UNKNOWN_TYPE_NAME, typeToken, "Unknown type name '" + typeName + "'.", hint);
    return Type::getError();
}

bool SemanticAnalyzer::isLValue(const Expr& expr) {
    // Only variables, member accesses, and array subscripts denote assignable memory locations
    return std::holds_alternative<std::unique_ptr<VariableExpr>>(expr.as) ||
           std::holds_alternative<std::unique_ptr<MemberAccessExpr>>(expr.as) ||
           std::holds_alternative<std::unique_ptr<ArrayAccessExpr>>(expr.as);
}

Token SemanticAnalyzer::getExprToken(const Expr& expr) {
    // Return the source anchor token for any given expression node
    return std::visit([](const auto& node) -> Token {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, std::unique_ptr<LiteralExpr>>) return node->value;
        if constexpr (std::is_same_v<T, std::unique_ptr<VariableExpr>>) return node->name;
        if constexpr (std::is_same_v<T, std::unique_ptr<ThisExpr>>) return node->keyword;
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
    // First sub-pass: Register type names to allow mutual references
    for (const auto& stmt : ast) {
        if (auto structStmt = std::get_if<std::unique_ptr<StructDeclStmt>>(&stmt.as)) {
            auto structType = std::make_unique<Type>();
            structType->kind = TypeKind::STRUCT;
            structType->name = (*structStmt)->name.lexeme;

            const Type* ptr = structType.get();
            customTypes.push_back(std::move(structType));

            if (!symbolTable.define(Symbol((*structStmt)->name.lexeme, ptr, SymbolKind::STRUCT, (*structStmt)->name))) {
                reportError(ErrorCode::E0004_DUPLICATE_DECLARATION, (*structStmt)->name, "Redefinition of struct '" + (*structStmt)->name.lexeme + "'.");
            }
        }
        else if (auto classStmt = std::get_if<std::unique_ptr<ClassDeclStmt>>(&stmt.as)) {
            auto classType = std::make_unique<Type>();
            classType->kind = TypeKind::CLASS;
            classType->name = (*classStmt)->name.lexeme;

            const Type* ptr = classType.get();
            customTypes.push_back(std::move(classType));

            if (!symbolTable.define(Symbol((*classStmt)->name.lexeme, ptr, SymbolKind::CLASS, (*classStmt)->name))) {
                reportError(ErrorCode::E0004_DUPLICATE_DECLARATION, (*classStmt)->name, "Redefinition of class '" + (*classStmt)->name.lexeme + "'.");
            }
        }
    }

    // Second sub-pass: Populate members, superclasses, and global function signatures
    for (const auto& stmt : ast) {
        if (auto structStmt = std::get_if<std::unique_ptr<StructDeclStmt>>(&stmt.as)) {
            auto symOpt = symbolTable.resolve((*structStmt)->name.lexeme);
            if (symOpt.has_value()) {
                Type* structType = const_cast<Type*>(symOpt->type);
                for (const auto& f : (*structStmt)->fields) {
                    if (auto varDecl = std::get_if<std::unique_ptr<VarDeclStmt>>(&f.as)) {
                        const Type* fType = resolveTypeFromToken((*varDecl)->type);
                        structType->fields[(*varDecl)->name.lexeme] = MemberInfo{fType, AccessModifier::PUBLIC};
                    }
                }
            }
        }
        else if (auto classStmt = std::get_if<std::unique_ptr<ClassDeclStmt>>(&stmt.as)) {
            auto symOpt = symbolTable.resolve((*classStmt)->name.lexeme);
            if (symOpt.has_value()) {
                Type* classType = const_cast<Type*>(symOpt->type);

                // Resolve superclass inheritance link
                if ((*classStmt)->superclass.has_value()) {
                    auto parentSym = symbolTable.resolve((*classStmt)->superclass->lexeme);
                    if (parentSym.has_value() && parentSym->kind == SymbolKind::CLASS) {
                        classType->superclass = parentSym->type;
                    } else {
                        reportError(ErrorCode::E0024_UNKNOWN_TYPE_NAME, *(*classStmt)->superclass, "Base class '" + (*classStmt)->superclass->lexeme + "' is not defined.");
                    }
                }

                // Register class fields and methods
                for (const auto& member : (*classStmt)->members) {
                    if (auto varDecl = std::get_if<std::unique_ptr<VarDeclStmt>>(&member.declaration.as)) {
                        const Type* fType = resolveTypeFromToken((*varDecl)->type);
                        classType->fields[(*varDecl)->name.lexeme] = MemberInfo{fType, member.access};
                    } else if (auto funcDecl = std::get_if<std::unique_ptr<FunctionDeclStmt>>(&member.declaration.as)) {
                        const Type* retType = resolveTypeFromToken((*funcDecl)->returnType);
                        std::vector<const Type*> pTypes;
                        for (const auto& p : (*funcDecl)->parameters) {
                            pTypes.push_back(resolveTypeFromToken(p.type));
                        }
                        classType->methods[(*funcDecl)->name.lexeme] = MemberInfo{
                            Type::makeFunction(retType, pTypes),
                            member.access
                        };
                    }
                }
            }
        }
        else if (auto funcStmt = std::get_if<std::unique_ptr<FunctionDeclStmt>>(&stmt.as)) {
            const Type* returnType = resolveTypeFromToken((*funcStmt)->returnType);
            std::vector<const Type*> paramTypes;
            for (const auto& p : (*funcStmt)->parameters) {
                paramTypes.push_back(resolveTypeFromToken(p.type));
            }
            const Type* funcType = Type::makeFunction(returnType, paramTypes);

            if (!symbolTable.define(Symbol((*funcStmt)->name.lexeme, funcType, SymbolKind::FUNCTION, (*funcStmt)->name))) {
                reportError(ErrorCode::E0004_DUPLICATE_DECLARATION, (*funcStmt)->name, "Redefinition of function '" + (*funcStmt)->name.lexeme + "'.");
            }
        }
    }
}

void SemanticAnalyzer::typeCheck(const std::vector<Stmt>& ast) {
    for (const auto& stmt : ast) {
        execute(stmt);
    }
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<LiteralExpr>& node) {
    switch (node->value.type) {
        case TokenType::INT_LITERAL:   return Type::getInt();
        case TokenType::FLOAT_LITERAL: return Type::getFloat();
        case TokenType::CHAR_LITERAL:  return Type::getChar();
        case TokenType::STRING_LITERAL:
        case TokenType::RAW_STRING_LITERAL: return Type::getString();
        case TokenType::KW_TRUE:
        case TokenType::KW_FALSE:      return Type::getBool();
        case TokenType::KW_NULL:       return Type::getVoid();
        default:                       return Type::getError();
    }
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<VariableExpr>& node) {
    // Resolve identifier in active lexical scopes
    auto symbolOpt = symbolTable.resolve(node->name.lexeme);
    if (!symbolOpt.has_value()) {
        std::string suggestion = diagnostics.findClosestMatch(node->name.lexeme, getVisibleSymbolNames());
        std::string hint = suggestion.empty() ? "" : "did you mean '" + suggestion + "'?";

        reportError(ErrorCode::E0001_UNDECLARED_VARIABLE, node->name,
                    "Variable '" + node->name.lexeme + "' is not declared in this scope.", hint);
        return Type::getError();
    }
    return symbolOpt->type;
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<ThisExpr>& node) {
    // 'this' is only valid inside class methods
    if (currentClass == nullptr) {
        reportError(ErrorCode::E0011_THIS_OUTSIDE_CLASS, node->keyword, "Cannot use 'this' outside of a class method.");
        return Type::getError();
    }
    return currentClass;
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<BinaryExpr>& node) {
    const Type* leftType = evaluate(node->left);
    const Type* rightType = evaluate(node->right);

    // Suppress secondary diagnostics on previously failed operands
    if (leftType->isError() || rightType->isError()) return Type::getError();

    // Assignment operator '='
    if (node->op.type == TokenType::OP_ASSIGN) {
        if (!isLValue(node->left)) {
            // Point to the left-hand expression that cannot be assigned
            reportError(ErrorCode::E0003_INVALID_ASSIGNMENT_TARGET, getExprToken(node->left),
                        "Invalid assignment target: left-hand side is not an assignable variable.");
            return Type::getError();
        }
        if (!rightType->isAssignableTo(leftType)) {
            // Point to the right-hand value being assigned
            reportError(ErrorCode::E0002_TYPE_MISMATCH, getExprToken(node->right),
                        "Cannot assign value of type '" + rightType->toString() +
                        "' to target of type '" + leftType->toString() + "'.");
            return Type::getError();
        }
        return leftType;
    }

    // Compound assignment operators (+=, -=, *=, /=, %=)
    if (node->op.type == TokenType::OP_PLUS_ASS || node->op.type == TokenType::OP_MIN_ASS ||
        node->op.type == TokenType::OP_MUL_ASS || node->op.type == TokenType::OP_DIV_ASS ||
        node->op.type == TokenType::OP_MOD_ASS) {
        if (!isLValue(node->left)) {
            reportError(ErrorCode::E0003_INVALID_ASSIGNMENT_TARGET, node->op, "Left-hand side of compound assignment is not assignable.");
            return Type::getError();
        }
        // Allow '+=' on strings for in-place concatenation
        if (node->op.type == TokenType::OP_PLUS_ASS && (leftType->isString() || rightType->isString())) {
            return leftType;
        }
        if (!leftType->isNumeric() || !rightType->isNumeric()) {
            reportError(ErrorCode::E0020_INVALID_OPERATOR_OPERANDS, node->op, "Compound operator requires numeric operands.");
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

        reportError(ErrorCode::E0020_INVALID_OPERATOR_OPERANDS, node->op, "Operator '+' cannot be applied between '" + leftType->toString() +
                              "' and '" + rightType->toString() + "'.");
        return Type::getError();
    }

    // Arithmetic operators (-, *, /, %)
    if (node->op.type == TokenType::OP_MINUS || node->op.type == TokenType::OP_STAR ||
        node->op.type == TokenType::OP_SLASH || node->op.type == TokenType::OP_MOD) {
        if (leftType->isInteger() && rightType->isInteger()) return Type::getInt();
        if (leftType->isNumeric() && rightType->isNumeric()) return Type::getFloat();

        reportError(ErrorCode::E0020_INVALID_OPERATOR_OPERANDS, node->op, "Arithmetic operator requires numeric operands.");
        return Type::getError();
    }

    // Equality comparisons (==, !=)
    if (node->op.type == TokenType::OP_EQ_EQ || node->op.type == TokenType::OP_NOT_EQ) {
        if (!leftType->isAssignableTo(rightType) && !rightType->isAssignableTo(leftType)) {
            reportError(ErrorCode::E0002_TYPE_MISMATCH, node->op, "Cannot compare incompatible types '" + leftType->toString() +
                                  "' and '" + rightType->toString() + "'.");
            return Type::getError();
        }
        return Type::getBool();
    }

    // Relational comparisons (<, <=, >, >=)
    if (node->op.type == TokenType::OP_LESS || node->op.type == TokenType::OP_LESS_EQ ||
        node->op.type == TokenType::OP_GRT || node->op.type == TokenType::OP_GRT_EQ) {
        if (!leftType->isNumeric() || !rightType->isNumeric()) {
            reportError(ErrorCode::E0020_INVALID_OPERATOR_OPERANDS, node->op, "Relational comparisons require numeric operands.");
            return Type::getError();
        }
        return Type::getBool();
    }

    // Logical boolean operators (&&, ||)
    if (node->op.type == TokenType::OP_AND || node->op.type == TokenType::OP_OR) {
        if (!leftType->isBool() || !rightType->isBool()) {
            reportError(ErrorCode::E0019_NON_BOOLEAN_CONDITION, node->op, "Logical operators require boolean operands.");
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
        reportError(ErrorCode::E0021_INVALID_RANGE_BOUNDS, node->op, "Range bounds must evaluate to integers.");
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
            reportError(ErrorCode::E0019_NON_BOOLEAN_CONDITION, node->op, "Logical NOT '!' requires a boolean operand.");
            return Type::getError();
        }
        return Type::getBool();
    }

    // Unary sign operators require numeric operand
    if (node->op.type == TokenType::OP_MINUS || node->op.type == TokenType::OP_PLUS) {
        if (!operandType->isNumeric()) {
            reportError(ErrorCode::E0020_INVALID_OPERATOR_OPERANDS, node->op, "Unary operator requires a numeric operand.");
            return Type::getError();
        }
        return operandType;
    }

    // Increment and decrement require numeric assignable target
    if (node->op.type == TokenType::OP_INC || node->op.type == TokenType::OP_DEC) {
        if (!isLValue(node->operand)) {
            // Point to the operand, not the operator
            reportError(ErrorCode::E0003_INVALID_ASSIGNMENT_TARGET, getExprToken(node->operand),
                        "Target of increment/decrement must be an assignable variable.");
            return Type::getError();
        }
        if (!operandType->isNumeric()) {
            reportError(ErrorCode::E0020_INVALID_OPERATOR_OPERANDS, node->op, "Increment/decrement requires a numeric operand.");
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
            reportError(ErrorCode::E0017_ARGUMENT_COUNT_MISMATCH, node->paren,
                        "Function expects at most " + std::to_string(totalParams) +
                        " arguments, but received " + std::to_string(passedArgs) + ".");
            return Type::getError();
        }

        // Validate argument type assignability
        for (size_t i = 0; i < passedArgs; ++i) {
            const Type* argType = evaluate(node->arguments[i]);
            const Type* paramType = calleeType->paramTypes[i];
            if (!argType->isAssignableTo(paramType)) {
                reportError(ErrorCode::E0018_ARGUMENT_TYPE_MISMATCH, node->paren,
                            "Argument " + std::to_string(i + 1) + " of type '" +
                            argType->toString() + "' is not assignable to parameter of type '" +
                            paramType->toString() + "'.");
            }
        }
        return calleeType->returnType;
    }

    reportError(ErrorCode::E0025_NOT_CALLABLE, node->paren, "Target expression is not callable as a function.");
    return Type::getError();
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<MemberAccessExpr>& node) {
    const Type* baseType = evaluate(node->accessed);
    if (baseType->isError()) return Type::getError();

    if (baseType->kind != TypeKind::CLASS && baseType->kind != TypeKind::STRUCT) {
        reportError(ErrorCode::E0013_MEMBER_NOT_FOUND, node->dot,
                    "Cannot access member '" + node->member.lexeme +
                    "' on non-class/non-struct type '" + baseType->toString() + "'.");
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
            reportError(ErrorCode::E0012_PRIVATE_MEMBER_ACCESS, node->member,
                        "Cannot access 'private' field '" + node->member.lexeme + "' of class '" + baseType->name + "'.",
                        "consider making '" + node->member.lexeme + "' public or providing a getter method.");
        }
        return fieldIt->second.type;
    }

    // Look up methods on the base class
    auto methodIt = baseType->methods.find(node->member.lexeme);
    if (methodIt != baseType->methods.end()) {
        if (!checkAccess(methodIt->second, baseType)) {
            reportError(ErrorCode::E0012_PRIVATE_MEMBER_ACCESS, node->member,
                        "Cannot access 'private' method '" + node->member.lexeme + "' of class '" + baseType->name + "'.");
        }
        return methodIt->second.type;
    }

    // Look up members in the superclass inheritance chain
    const Type* parent = baseType->superclass;
    while (parent != nullptr) {
        auto pField = parent->fields.find(node->member.lexeme);
        if (pField != parent->fields.end()) {
            if (!checkAccess(pField->second, parent)) {
                reportError(ErrorCode::E0012_PRIVATE_MEMBER_ACCESS, node->member, "Cannot access 'private' member of base class.");
            }
            return pField->second.type;
        }
        auto pMethod = parent->methods.find(node->member.lexeme);
        if (pMethod != parent->methods.end()) {
            if (!checkAccess(pMethod->second, parent)) {
                reportError(ErrorCode::E0012_PRIVATE_MEMBER_ACCESS, node->member, "Cannot access 'private' method of base class.");
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

    reportError(ErrorCode::E0013_MEMBER_NOT_FOUND, node->member,
                "Type '" + baseType->name + "' has no member named '" + node->member.lexeme + "'.", hint);
    return Type::getError();
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<ArrayAccessExpr>& node) {
    const Type* arrayType = evaluate(node->array);
    const Type* indexType = evaluate(node->index);

    if (!indexType->isInteger()) {
        reportError(ErrorCode::E0015_INVALID_SUBSCRIPT_INDEX, node->openingBracket, "Array index must evaluate to an integer.");
    }

    if (arrayType->kind != TypeKind::ARRAY) {
        reportError(ErrorCode::E0014_INVALID_SUBSCRIPT_TARGET, node->openingBracket, "Subscript operator '[]' can only be applied to array types.");
        return Type::getError();
    }

    return arrayType->elementType;
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<ArrayLiteralExpr>& node) {
    if (node->elements.empty()) {
        return Type::makeArray(Type::getVoid());
    }

    // Infer array element type from the first element and check homogeneity
    const Type* firstType = evaluate(node->elements[0]);
    for (size_t i = 1; i < node->elements.size(); ++i) {
        const Type* elemType = evaluate(node->elements[i]);
        if (!elemType->isAssignableTo(firstType)) {
            // Point to the specific element at index i, not the opening bracket '['
            reportError(ErrorCode::E0016_ARRAY_ELEMENT_TYPE_MISMATCH, getExprToken(node->elements[i]),
                        "Array element of type '" + elemType->toString() +
                        "' is incompatible with array element type '" + firstType->toString() + "'.");
        }
    }
    return Type::makeArray(firstType);
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<TernaryExpr>& node) {
    const Type* condType = evaluate(node->condition);
    if (!condType->isBool()) {
        reportError(ErrorCode::E0019_NON_BOOLEAN_CONDITION, node->questionMark, "Ternary condition must evaluate to a boolean expression.");
    }

    const Type* trueType = evaluate(node->trueBranch);
    const Type* falseType = evaluate(node->falseBranch);

    if (!falseType->isAssignableTo(trueType) && !trueType->isAssignableTo(falseType)) {
        reportError(ErrorCode::E0022_INCOMPATIBLE_TERNARY_BRANCHES, node->questionMark,
                    "Ternary branch types are incompatible ('" +
                    trueType->toString() + "' vs '" + falseType->toString() + "').");
        return Type::getError();
    }

    return trueType;
}

void SemanticAnalyzer::operator()(const std::unique_ptr<ExpressionStmt>& node) {
    evaluate(node->expression);
}

void SemanticAnalyzer::operator()(const std::unique_ptr<VarDeclStmt>& node) {
    const Type* declaredType = resolveTypeFromToken(node->type);

    // Validate initializer expression assignability
    if (node->initializer != nullptr) {
        const Type* initType = evaluate(*node->initializer);
        if (!initType->isAssignableTo(declaredType)) {
            // Point directly to the initializer expression!
            reportError(ErrorCode::E0002_TYPE_MISMATCH, getExprToken(*node->initializer),
                        "Cannot initialize variable of type '" + declaredType->toString() +
                        "' with expression of type '" + initType->toString() + "'.");
        }
    }

    // Register variable in current lexical scope
    if (!symbolTable.define(Symbol(node->name.lexeme, declaredType, SymbolKind::VARIABLE, node->name))) {
        reportError(ErrorCode::E0004_DUPLICATE_DECLARATION, node->name, "Variable '" + node->name.lexeme + "' is already defined in this block.");
    }
}

void SemanticAnalyzer::operator()(const std::unique_ptr<BlockStmt>& node) {
    symbolTable.enterScope(ScopeKind::BLOCK);
    for (const auto& stmt : node->statements) {
        execute(stmt);
    }
    symbolTable.exitScope();
}

void SemanticAnalyzer::operator()(const std::unique_ptr<IfStmt>& node) {
    const Type* condType = evaluate(node->condition);
    if (!condType->isBool()) {
        reportError(ErrorCode::E0019_NON_BOOLEAN_CONDITION, node->keyword, "If condition must be a boolean expression.");
    }

    execute(node->thenBranch);

    for (const auto& elifBranch : node->elifBranches) {
        const Type* elifCondType = evaluate(elifBranch.condition);
        if (!elifCondType->isBool()) {
            reportError(ErrorCode::E0019_NON_BOOLEAN_CONDITION, elifBranch.keyword, "Elif condition must be a boolean expression.");
        }
        execute(elifBranch.block);
    }

    if (node->elseBranch != nullptr) {
        execute(*node->elseBranch);
    }
}

void SemanticAnalyzer::operator()(const std::unique_ptr<WhileStmt>& node) {
    const Type* condType = evaluate(node->condition);
    if (!condType->isBool()) {
        reportError(ErrorCode::E0019_NON_BOOLEAN_CONDITION, node->keyword, "While loop condition must be a boolean expression.");
    }

    loopDepth++;
    execute(node->body);
    loopDepth--;
}

void SemanticAnalyzer::operator()(const std::unique_ptr<ForStmt>& node) {
    const Type* iterableType = evaluate(node->iterable);
    if (iterableType->kind != TypeKind::ARRAY) {
        reportError(ErrorCode::E0020_INVALID_OPERATOR_OPERANDS, node->keyword, "For loop iterable must evaluate to an array or range.");
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
                reportError(ErrorCode::E0002_TYPE_MISMATCH, param.name,
                            "Default value of type '" + defType->toString() +
                            "' does not match parameter type '" + paramType->toString() + "'.");
            }
        } else if (seenDefault) {
            // Positional parameters cannot follow parameters with default values
            reportError(ErrorCode::E0023_DEFAULT_PARAM_ORDER, param.name,
                        "Positional parameter '" + param.name.lexeme +
                        "' cannot follow default parameters.");
        }

        symbolTable.define(Symbol(param.name.lexeme, paramType, SymbolKind::PARAMETER, param.name));
    }

    execute(node->body);

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
        reportError(ErrorCode::E0007_RETURN_OUTSIDE_FUNCTION, node->keyword, "'return' statement not allowed outside of a function.");
        return;
    }

    if (currentFunctionReturnType->isVoid()) {
        if (node->value != nullptr) {
            reportError(ErrorCode::E0010_RETURN_IN_VOID_FUNCTION, node->keyword, "Cannot return a value from a function with 'void' return type.");
        }
    } else {
        if (node->value == nullptr) {
            reportError(ErrorCode::E0009_MISSING_RETURN_VALUE, node->keyword, "Missing return value: function expects return type '" +
                                       currentFunctionReturnType->toString() + "'.");
        } else {
            const Type* valType = evaluate(*node->value);
            if (!valType->isAssignableTo(currentFunctionReturnType)) {
                reportError(ErrorCode::E0008_RETURN_TYPE_MISMATCH, node->keyword, "Return value of type '" + valType->toString() +
                                           "' does not match expected function return type '" +
                                           currentFunctionReturnType->toString() + "'.");
            }
        }
    }
}

void SemanticAnalyzer::operator()(const std::unique_ptr<BreakStmt>& node) {
    if (loopDepth == 0) {
        reportError(ErrorCode::E0005_BREAK_OUTSIDE_LOOP, node->keyword, "'break' statement is only allowed inside loops.");
    }
}

void SemanticAnalyzer::operator()(const std::unique_ptr<ContinueStmt>& node) {
    if (loopDepth == 0) {
        reportError(ErrorCode::E0006_CONTINUE_OUTSIDE_LOOP, node->keyword, "'continue' statement is only allowed inside loops.");
    }
}

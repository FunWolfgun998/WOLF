#include "../../include/Semantic/SemanticAnalyzer.h"
#include <iostream>

SemanticAnalyzer::SemanticAnalyzer() {}

void SemanticAnalyzer::reportError(const Token& token, const std::string& message) {
    // Format error string using line and column from token for IDE/console output
    std::string err = "[Semantic Error] Line " + std::to_string(token.line) +
                      ", Col " + std::to_string(token.column) + ": " + message;
    errors.push_back(err);
}

const Type* SemanticAnalyzer::resolveTypeFromToken(const Token& typeToken) {
    std::string typeName = typeToken.lexeme;

    // Match built-in primitive type singletons
    if (typeName == "int") return Type::getInt();
    if (typeName == "float") return Type::getFloat();
    if (typeName == "char") return Type::getChar();
    if (typeName == "string") return Type::getString();
    if (typeName == "bool") return Type::getBool();
    if (typeName == "void") return Type::getVoid();

    // Strip trailing '[]' to recursively resolve multi-dimensional array types
    if (typeName.size() > 2 && typeName.substr(typeName.size() - 2) == "[]") {
        Token baseToken = typeToken;
        baseToken.lexeme = typeName.substr(0, typeName.size() - 2);
        const Type* baseType = resolveTypeFromToken(baseToken);
        return Type::makeArray(baseType);
    }

    // Resolve user-defined types (structs and classes) from active symbol table
    auto symbolOpt = symbolTable.resolve(typeName);
    if (symbolOpt.has_value()) {
        // Ensure the symbol is actually a type definition and not a local variable
        if (symbolOpt->kind == SymbolKind::CLASS || symbolOpt->kind == SymbolKind::STRUCT) {
            return symbolOpt->type;
        }
        reportError(typeToken, "'" + typeName + "' is a variable name, not a type.");
        return Type::getError();
    }

    reportError(typeToken, "Unknown type name '" + typeName + "'.");
    return Type::getError();
}

bool SemanticAnalyzer::isLValue(const Expr& expr) const {
    // Only variables, object fields, and array subscript locations can be assigned to
    return std::holds_alternative<std::unique_ptr<VariableExpr>>(expr.as) ||
           std::holds_alternative<std::unique_ptr<MemberAccessExpr>>(expr.as) ||
           std::holds_alternative<std::unique_ptr<ArrayAccessExpr>>(expr.as);
}

const Type* SemanticAnalyzer::evaluate(const Expr& expr) {
    // Dispatch expression visitor via std::visit and return inferred type
    return std::visit(*this, expr.as);
}

void SemanticAnalyzer::execute(const Stmt& stmt) {
    // Dispatch statement visitor via std::visit
    std::visit(*this, stmt.as);
}

bool SemanticAnalyzer::analyze(const std::vector<Stmt>& ast) {
    errors.clear();

    // Pass 1: Collect global type and function declarations for forward reference support
    collectDeclarations(ast);

    // Pass 2: Perform deep semantic validation and type inference across all bodies
    typeCheck(ast);

    return !hasErrors();
}

void SemanticAnalyzer::collectDeclarations(const std::vector<Stmt>& ast) {
    for (const auto& stmt : ast) {
        // Register top-level struct declarations
        if (auto structStmt = std::get_if<std::unique_ptr<StructDeclStmt>>(&stmt.as)) {
            auto structType = std::make_unique<Type>();
            structType->kind = TypeKind::STRUCT;
            structType->name = (*structStmt)->name.lexeme;

            const Type* ptr = structType.get();
            customTypes.push_back(std::move(structType));

            // Define struct name in global symbol table
            if (!symbolTable.define(Symbol((*structStmt)->name.lexeme, ptr, SymbolKind::STRUCT, (*structStmt)->name))) {
                reportError((*structStmt)->name, "Redefinition of struct '" + (*structStmt)->name.lexeme + "'.");
            }
        }
        // Register top-level class declarations
        else if (auto classStmt = std::get_if<std::unique_ptr<ClassDeclStmt>>(&stmt.as)) {
            auto classType = std::make_unique<Type>();
            classType->kind = TypeKind::CLASS;
            classType->name = (*classStmt)->name.lexeme;

            const Type* ptr = classType.get();
            customTypes.push_back(std::move(classType));

            // Define class name in global symbol table
            if (!symbolTable.define(Symbol((*classStmt)->name.lexeme, ptr, SymbolKind::CLASS, (*classStmt)->name))) {
                reportError((*classStmt)->name, "Redefinition of class '" + (*classStmt)->name.lexeme + "'.");
            }
        }
        // Register top-level function signatures
        else if (auto funcStmt = std::get_if<std::unique_ptr<FunctionDeclStmt>>(&stmt.as)) {
            const Type* returnType = resolveTypeFromToken((*funcStmt)->returnType);
            std::vector<const Type*> paramTypes;
            for (const auto& p : (*funcStmt)->parameters) {
                paramTypes.push_back(resolveTypeFromToken(p.type));
            }
            const Type* funcType = Type::makeFunction(returnType, paramTypes);

            // Define function signature in global symbol table
            if (!symbolTable.define(Symbol((*funcStmt)->name.lexeme, funcType, SymbolKind::FUNCTION, (*funcStmt)->name))) {
                reportError((*funcStmt)->name, "Redefinition of function '" + (*funcStmt)->name.lexeme + "'.");
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
    // Map AST literal token types to corresponding primitive Type pointers
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
    // Look up variable in active lexical scope hierarchy
    auto symbolOpt = symbolTable.resolve(node->name.lexeme);
    if (!symbolOpt.has_value()) {
        reportError(node->name, "Variable '" + node->name.lexeme + "' is not declared in this scope.");
        return Type::getError();
    }
    return symbolOpt->type;
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<ThisExpr>& node) {
    // Disallow 'this' outside of class methods
    if (currentClass == nullptr) {
        reportError(node->keyword, "Cannot use 'this' outside of a class method.");
        return Type::getError();
    }
    return currentClass;
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<BinaryExpr>& node) {
    // Infer types of both operands first (bottom-up traversal)
    const Type* leftType = evaluate(node->left);
    const Type* rightType = evaluate(node->right);

    // Suppress further errors if any operand already failed type inference
    if (leftType->isError() || rightType->isError()) return Type::getError();

    // Handle simple assignment '='
    if (node->op.type == TokenType::OP_ASSIGN) {
        // Enforce assignability of left-hand side
        if (!isLValue(node->left)) {
            reportError(node->op, "Invalid assignment target: left-hand side is not an assignable variable.");
            return Type::getError();
        }
        // Verify type compatibility
        if (!rightType->isAssignableTo(leftType)) {
            reportError(node->op, "Cannot assign value of type '" + rightType->toString() +
                                  "' to target of type '" + leftType->toString() + "'.");
            return Type::getError();
        }
        return leftType;
    }

    // Handle compound assignments (+=, -=, *=, /=, %=)
    if (node->op.type == TokenType::OP_PLUS_ASS || node->op.type == TokenType::OP_MIN_ASS ||
        node->op.type == TokenType::OP_MUL_ASS || node->op.type == TokenType::OP_DIV_ASS ||
        node->op.type == TokenType::OP_MOD_ASS) {
        if (!isLValue(node->left)) {
            reportError(node->op, "Left-hand side of compound assignment is not assignable.");
            return Type::getError();
        }
        // Allow '+=' on strings for in-place concatenation
        if (node->op.type == TokenType::OP_PLUS_ASS && (leftType->isString() || rightType->isString())) {
            return leftType;
        }
        // Arithmetic compound operators require numeric types
        if (!leftType->isNumeric() || !rightType->isNumeric()) {
            reportError(node->op, "Compound operator requires numeric operands.");
            return Type::getError();
        }
        return leftType;
    }

    // Handle addition '+' and string concatenation
    if (node->op.type == TokenType::OP_PLUS) {
        // If either operand is a string, promote operation to string concatenation
        if (leftType->isString() || rightType->isString()) {
            return Type::getString();
        }
        // Both integers yield integer
        if (leftType->isInteger() && rightType->isInteger()) return Type::getInt();
        // Mixed float/int or both floats yield float
        if (leftType->isNumeric() && rightType->isNumeric()) return Type::getFloat();

        reportError(node->op, "Operator '+' cannot be applied between '" + leftType->toString() +
                              "' and '" + rightType->toString() + "'.");
        return Type::getError();
    }

    // Handle arithmetic operators (-, *, /, %)
    if (node->op.type == TokenType::OP_MINUS || node->op.type == TokenType::OP_STAR ||
        node->op.type == TokenType::OP_SLASH || node->op.type == TokenType::OP_MOD) {
        if (leftType->isInteger() && rightType->isInteger()) return Type::getInt();
        if (leftType->isNumeric() && rightType->isNumeric()) return Type::getFloat();

        reportError(node->op, "Arithmetic operator requires numeric operands.");
        return Type::getError();
    }

    // Handle equality comparisons (==, !=)
    if (node->op.type == TokenType::OP_EQ_EQ || node->op.type == TokenType::OP_NOT_EQ) {
        if (!leftType->isAssignableTo(rightType) && !rightType->isAssignableTo(leftType)) {
            reportError(node->op, "Cannot compare incompatible types '" + leftType->toString() +
                                  "' and '" + rightType->toString() + "'.");
            return Type::getError();
        }
        return Type::getBool();
    }

    // Handle relational comparisons (<, <=, >, >=)
    if (node->op.type == TokenType::OP_LESS || node->op.type == TokenType::OP_LESS_EQ ||
        node->op.type == TokenType::OP_GRT || node->op.type == TokenType::OP_GRT_EQ) {
        if (!leftType->isNumeric() || !rightType->isNumeric()) {
            reportError(node->op, "Relational comparisons require numeric operands.");
            return Type::getError();
        }
        return Type::getBool();
    }

    // Handle logical boolean operators (&&, ||)
    if (node->op.type == TokenType::OP_AND || node->op.type == TokenType::OP_OR) {
        if (!leftType->isBool() || !rightType->isBool()) {
            reportError(node->op, "Logical operators require boolean operands.");
            return Type::getError();
        }
        return Type::getBool();
    }

    return Type::getError();
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<RangeExpr>& node) {
    // Both start and end bounds of '..' must evaluate to integers
    const Type* startType = evaluate(node->start);
    const Type* endType = evaluate(node->end);

    if (!startType->isInteger() || !endType->isInteger()) {
        reportError(node->op, "Range bounds must evaluate to integers.");
        return Type::getError();
    }
    // A range behaves like an iterable integer array
    return Type::makeArray(Type::getInt());
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<UnaryExpr>& node) {
    const Type* operandType = evaluate(node->operand);
    if (operandType->isError()) return Type::getError();

    // Logical negation '!' requires boolean
    if (node->op.type == TokenType::OP_NOT) {
        if (!operandType->isBool()) {
            reportError(node->op, "Logical NOT '!' requires a boolean operand.");
            return Type::getError();
        }
        return Type::getBool();
    }

    // Unary sign operators (+, -) require numeric operand
    if (node->op.type == TokenType::OP_MINUS || node->op.type == TokenType::OP_PLUS) {
        if (!operandType->isNumeric()) {
            reportError(node->op, "Unary operator requires a numeric operand.");
            return Type::getError();
        }
        return operandType;
    }

    // Increment and decrement (++, --) require both numeric operand and assignable location
    if (node->op.type == TokenType::OP_INC || node->op.type == TokenType::OP_DEC) {
        if (!isLValue(node->operand)) {
            reportError(node->op, "Target of increment/decrement must be an assignable variable.");
            return Type::getError();
        }
        if (!operandType->isNumeric()) {
            reportError(node->op, "Increment/decrement requires a numeric operand.");
            return Type::getError();
        }
        return operandType;
    }

    return Type::getError();
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<GroupingExpr>& node) {
    // Parentheses simply forward the evaluated inner expression type
    return evaluate(node->expression);
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<CallExpr>& node) {
    const Type* calleeType = evaluate(node->callee);
    if (calleeType->isError()) return Type::getError();

    // Instantiating a struct or class (constructor invocation) returns the type instance
    if (calleeType->kind == TypeKind::CLASS || calleeType->kind == TypeKind::STRUCT) {
        return calleeType;
    }

    // Standard function/method call validation
    if (calleeType->kind == TypeKind::FUNCTION) {
        // Check argument count against expected parameters
        if (node->arguments.size() != calleeType->paramTypes.size()) {
            reportError(node->paren, "Function expects " + std::to_string(calleeType->paramTypes.size()) +
                                     " arguments, but received " + std::to_string(node->arguments.size()) + ".");
        } else {
            // Check individual argument type assignability
            for (size_t i = 0; i < node->arguments.size(); ++i) {
                const Type* argType = evaluate(node->arguments[i]);
                const Type* paramType = calleeType->paramTypes[i];
                if (!argType->isAssignableTo(paramType)) {
                    reportError(node->paren, "Argument " + std::to_string(i + 1) + " of type '" +
                                             argType->toString() + "' is not assignable to parameter of type '" +
                                             paramType->toString() + "'.");
                }
            }
        }
        return calleeType->returnType;
    }

    reportError(node->paren, "Target expression is not callable as a function.");
    return Type::getError();
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<MemberAccessExpr>& node) {
    const Type* baseType = evaluate(node->accessed);
    if (baseType->isError()) return Type::getError();

    // Member access '.' is only valid on classes and structs
    if (baseType->kind != TypeKind::CLASS && baseType->kind != TypeKind::STRUCT) {
        reportError(node->dot, "Cannot access member '" + node->member.lexeme +
                               "' on non-class/non-struct type '" + baseType->toString() + "'.");
        return Type::getError();
    }

    // Search fields in base type
    auto fieldIt = baseType->fields.find(node->member.lexeme);
    if (fieldIt != baseType->fields.end()) {
        return fieldIt->second;
    }

    // Search methods in base type
    auto methodIt = baseType->methods.find(node->member.lexeme);
    if (methodIt != baseType->methods.end()) {
        return methodIt->second;
    }

    // Traverse inheritance chain if member is defined in superclass
    const Type* parent = baseType->superclass;
    while (parent != nullptr) {
        auto pField = parent->fields.find(node->member.lexeme);
        if (pField != parent->fields.end()) return pField->second;
        auto pMethod = parent->methods.find(node->member.lexeme);
        if (pMethod != parent->methods.end()) return pMethod->second;
        parent = parent->superclass;
    }

    reportError(node->member, "Type '" + baseType->name + "' has no member named '" + node->member.lexeme + "'.");
    return Type::getError();
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<ArrayAccessExpr>& node) {
    const Type* arrayType = evaluate(node->array);
    const Type* indexType = evaluate(node->index);

    // Array subscripts must be integers
    if (!indexType->isInteger()) {
        reportError(node->openingBracket, "Array index must evaluate to an integer.");
    }

    // Base expression must be an array type
    if (arrayType->kind != TypeKind::ARRAY) {
        reportError(node->openingBracket, "Subscript operator '[]' can only be applied to array types.");
        return Type::getError();
    }

    return arrayType->elementType;
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<ArrayLiteralExpr>& node) {
    if (node->elements.empty()) {
        return Type::makeArray(Type::getVoid());
    }

    // Infer array element type from the first element
    const Type* firstType = evaluate(node->elements[0]);
    for (size_t i = 1; i < node->elements.size(); ++i) {
        const Type* elemType = evaluate(node->elements[i]);
        // Enforce homogeneous array element types
        if (!elemType->isAssignableTo(firstType)) {
            reportError(node->openingBracket, "Array element at index " + std::to_string(i) +
                                              " of type '" + elemType->toString() +
                                              "' is incompatible with array element type '" + firstType->toString() + "'.");
        }
    }

    return Type::makeArray(firstType);
}

const Type* SemanticAnalyzer::operator()(const std::unique_ptr<TernaryExpr>& node) {
    // Condition must evaluate to boolean
    const Type* condType = evaluate(node->condition);
    if (!condType->isBool()) {
        reportError(node->questionMark, "Ternary condition must evaluate to a boolean expression.");
    }

    // Both branch types must be mutually compatible
    const Type* trueType = evaluate(node->trueBranch);
    const Type* falseType = evaluate(node->falseBranch);

    if (!falseType->isAssignableTo(trueType) && !trueType->isAssignableTo(falseType)) {
        reportError(node->questionMark, "Ternary branch types are incompatible ('" +
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

    // Validate initializer expression if present
    if (node->initializer != nullptr) {
        const Type* initType = evaluate(*node->initializer);
        if (!initType->isAssignableTo(declaredType)) {
            reportError(node->name, "Cannot initialize variable of type '" + declaredType->toString() +
                                    "' with expression of type '" + initType->toString() + "'.");
        }
    }

    // Register variable in current active scope; reject redefinitions in the same block
    if (!symbolTable.define(Symbol(node->name.lexeme, declaredType, SymbolKind::VARIABLE, node->name))) {
        reportError(node->name, "Variable '" + node->name.lexeme + "' is already defined in this block.");
    }
}

void SemanticAnalyzer::operator()(const std::unique_ptr<BlockStmt>& node) {
    // Open isolated block scope
    symbolTable.enterScope(ScopeKind::BLOCK);
    for (const auto& stmt : node->statements) {
        execute(stmt);
    }
    // Close block scope and discard local symbols
    symbolTable.exitScope();
}

void SemanticAnalyzer::operator()(const std::unique_ptr<IfStmt>& node) {
    // Validate if-condition type
    const Type* condType = evaluate(node->condition);
    if (!condType->isBool()) {
        reportError(node->keyword, "If condition must be a boolean expression.");
    }

    execute(node->thenBranch);

    // Validate all elif branch conditions and bodies
    for (const auto& elifBranch : node->elifBranches) {
        const Type* elifCondType = evaluate(elifBranch.condition);
        if (!elifCondType->isBool()) {
            reportError(elifBranch.keyword, "Elif condition must be a boolean expression.");
        }
        execute(elifBranch.block);
    }

    // Validate optional else branch
    if (node->elseBranch != nullptr) {
        execute(*node->elseBranch);
    }
}

void SemanticAnalyzer::operator()(const std::unique_ptr<WhileStmt>& node) {
    // Validate while-condition type
    const Type* condType = evaluate(node->condition);
    if (!condType->isBool()) {
        reportError(node->keyword, "While loop condition must be a boolean expression.");
    }

    // Track loop depth to allow break/continue
    loopDepth++;
    execute(node->body);
    loopDepth--;
}

void SemanticAnalyzer::operator()(const std::unique_ptr<ForStmt>& node) {
    const Type* iterableType = evaluate(node->iterable);
    // For loop requires an array or range
    if (iterableType->kind != TypeKind::ARRAY) {
        reportError(node->keyword, "For loop iterable must evaluate to an array or range.");
    }

    loopDepth++;
    symbolTable.enterScope(ScopeKind::BLOCK);

    // Define iterator variable in local loop scope
    const Type* elemType = iterableType->elementType ? iterableType->elementType : Type::getInt();
    symbolTable.define(Symbol(node->iteratorVar.lexeme, elemType, SymbolKind::VARIABLE, node->iteratorVar));

    execute(node->body);

    symbolTable.exitScope();
    loopDepth--;
}

void SemanticAnalyzer::operator()(const std::unique_ptr<FunctionDeclStmt>& node) {
    const Type* returnType = resolveTypeFromToken(node->returnType);
    currentFunctionReturnType = returnType;

    // Open dedicated function scope
    symbolTable.enterScope(ScopeKind::FUNCTION);

    bool seenDefault = false;
    for (const auto& param : node->parameters) {
        const Type* paramType = resolveTypeFromToken(param.type);

        // Check default parameter expression and ordering
        if (param.defaultValue != nullptr) {
            seenDefault = true;
            const Type* defType = evaluate(*param.defaultValue);
            if (!defType->isAssignableTo(paramType)) {
                reportError(param.name, "Default value type '" + defType->toString() +
                                        "' does not match parameter type '" + paramType->toString() + "'.");
            }
        } else if (seenDefault) {
            // Reject parameters without default value that follow default parameters
            reportError(param.name, "Positional parameter '" + param.name.lexeme +
                                    "' cannot follow default parameters.");
        }

        // Define parameter in local function scope
        symbolTable.define(Symbol(param.name.lexeme, paramType, SymbolKind::PARAMETER, param.name));
    }

    execute(node->body);

    symbolTable.exitScope();
    currentFunctionReturnType = nullptr;
}

void SemanticAnalyzer::operator()(const std::unique_ptr<StructDeclStmt>& node) {
    // Populate struct field metadata for member access resolution
    auto symOpt = symbolTable.resolve(node->name.lexeme);
    if (symOpt.has_value()) {
        Type* structType = const_cast<Type*>(symOpt->type);
        for (const auto& f : node->fields) {
            if (auto varDecl = std::get_if<std::unique_ptr<VarDeclStmt>>(&f.as)) {
                const Type* fType = resolveTypeFromToken((*varDecl)->type);
                structType->fields[(*varDecl)->name.lexeme] = fType;
            }
        }
    }
}

void SemanticAnalyzer::operator()(const std::unique_ptr<ClassDeclStmt>& node) {
    auto symOpt = symbolTable.resolve(node->name.lexeme);
    if (!symOpt.has_value()) return;

    Type* classType = const_cast<Type*>(symOpt->type);

    // Resolve superclass if inheritance was specified
    if (node->superclass.has_value()) {
        auto parentSym = symbolTable.resolve(node->superclass->lexeme);
        if (parentSym.has_value() && parentSym->kind == SymbolKind::CLASS) {
            classType->superclass = parentSym->type;
        } else {
            reportError(*node->superclass, "Base class '" + node->superclass->lexeme + "' is not defined.");
        }
    }

    // Populate class fields and methods metadata
    for (const auto& member : node->members) {
        if (auto varDecl = std::get_if<std::unique_ptr<VarDeclStmt>>(&member.declaration.as)) {
            const Type* fType = resolveTypeFromToken((*varDecl)->type);
            classType->fields[(*varDecl)->name.lexeme] = fType;
        } else if (auto funcDecl = std::get_if<std::unique_ptr<FunctionDeclStmt>>(&member.declaration.as)) {
            const Type* retType = resolveTypeFromToken((*funcDecl)->returnType);
            std::vector<const Type*> pTypes;
            for (const auto& p : (*funcDecl)->parameters) {
                pTypes.push_back(resolveTypeFromToken(p.type));
            }
            classType->methods[(*funcDecl)->name.lexeme] = Type::makeFunction(retType, pTypes);
        }
    }

    // Analyze method bodies inside the active class context
    currentClass = classType;
    symbolTable.enterScope(ScopeKind::CLASS);

    for (const auto& member : node->members) {
        execute(member.declaration);
    }

    symbolTable.exitScope();
    currentClass = nullptr;
}

void SemanticAnalyzer::operator()(const std::unique_ptr<ReturnStmt>& node) {
    // Disallow return statements outside functions
    if (currentFunctionReturnType == nullptr) {
        reportError(node->keyword, "'return' statement not allowed outside of a function.");
        return;
    }

    // Verify void return expectations
    if (currentFunctionReturnType->isVoid()) {
        if (node->value != nullptr) {
            reportError(node->keyword, "Cannot return a value from a function with 'void' return type.");
        }
    } else {
        // Enforce return value presence and type compatibility for non-void functions
        if (node->value == nullptr) {
            reportError(node->keyword, "Missing return value: function expects return type '" +
                                       currentFunctionReturnType->toString() + "'.");
        } else {
            const Type* valType = evaluate(*node->value);
            if (!valType->isAssignableTo(currentFunctionReturnType)) {
                reportError(node->keyword, "Return value of type '" + valType->toString() +
                                           "' does not match expected function return type '" +
                                           currentFunctionReturnType->toString() + "'.");
            }
        }
    }
}

void SemanticAnalyzer::operator()(const std::unique_ptr<BreakStmt>& node) {
    // Disallow break outside loop bodies
    if (loopDepth == 0) {
        reportError(node->keyword, "'break' statement is only allowed inside loops.");
    }
}

void SemanticAnalyzer::operator()(const std::unique_ptr<ContinueStmt>& node) {
    // Disallow continue outside loop bodies
    if (loopDepth == 0) {
        reportError(node->keyword, "'continue' statement is only allowed inside loops.");
    }
}
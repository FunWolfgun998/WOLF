#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "../AST/ASTNodes.h" // For AccessModifier enum

enum class TypeKind {
    PRIMITIVE,
    ARRAY,
    FUNCTION,
    STRUCT,
    CLASS,
    VOID_TYPE,
    NULL_TYPE,
    ERROR_TYPE
};

struct Type;

// Detailed metadata for class fields and methods
struct MemberInfo {
    const Type* type = nullptr;
    AccessModifier access = AccessModifier::PUBLIC;
    bool isStatic = false;
    bool isAbstract = false;
};

struct Type {
    TypeKind kind;
    std::string name;
    bool isNullable = false;
    bool isAbstract = false;

    // Direct pointer to the underlying non-nullable base type (if isNullable is true)
    const Type* nonNullableType = nullptr;

    // For Arrays: element type (e.g. 'int' for int[])
    const Type* elementType = nullptr;

    // For Functions: return type, parameters, and minimum required arguments (without defaults)
    const Type* returnType = nullptr;
    std::vector<const Type*> paramTypes;
    size_t minArity = 0;

    // For Classes: inheritance link
    const Type* superclass = nullptr;

    // For Classes and Structs: members with access modifiers
    std::unordered_map<std::string, MemberInfo> fields;
    std::unordered_map<std::string, MemberInfo> methods;

    // --- Helper Queries ---
    bool isInteger() const { return kind == TypeKind::PRIMITIVE && name == "int"; }
    bool isFloat() const   { return kind == TypeKind::PRIMITIVE && name == "float"; }
    bool isChar() const    { return kind == TypeKind::PRIMITIVE && name == "char"; }
    bool isString() const  { return kind == TypeKind::PRIMITIVE && name == "string"; }
    bool isBool() const    { return kind == TypeKind::PRIMITIVE && name == "bool"; }
    bool isVoid() const    { return kind == TypeKind::VOID_TYPE; }
    bool isError() const   { return kind == TypeKind::ERROR_TYPE; }
    bool isNumeric() const { return isInteger() || isFloat(); }
    bool isNull() const { return kind == TypeKind::NULL_TYPE; }

    bool isAssignableTo(const Type* target) const;
    std::string toString() const;

    // Factory Singletons
    static const Type* getInt();
    static const Type* getFloat();
    static const Type* getChar();
    static const Type* getString();
    static const Type* getBool();
    static const Type* getVoid();
    static const Type* getNull();
    static const Type* getError();

    // Complex Types Factory
    static const Type* makeArray(const Type* element);
    static const Type* makeFunction(const Type* returnType, std::vector<const Type*> params, size_t minArity = 0);
    static const Type* makeNullable(const Type* base);
    static const Type* getNonNullable(const Type* type);
};
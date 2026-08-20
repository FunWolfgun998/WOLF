#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

enum class TypeKind {
    PRIMITIVE,
    ARRAY,
    FUNCTION,
    STRUCT,
    CLASS,
    VOID_TYPE,
    ERROR_TYPE // Sentinel type to prevent cascading errors
};

struct Type {
    TypeKind kind;
    std::string name;

    // For Arrays: the type of elements inside (e.g. 'int' for int[])
    const Type* elementType = nullptr;

    // For Functions: return type and parameter types
    const Type* returnType = nullptr;
    std::vector<const Type*> paramTypes;

    // For Classes: optional superclass for inheritance
    const Type* superclass = nullptr;

    // For Classes and Structs: fields and methods
    std::unordered_map<std::string, const Type*> fields;
    std::unordered_map<std::string, const Type*> methods;

    // --- Helper Queries ---
    bool isInteger() const { return kind == TypeKind::PRIMITIVE && name == "int"; }
    bool isFloat() const   { return kind == TypeKind::PRIMITIVE && name == "float"; }
    bool isChar() const    { return kind == TypeKind::PRIMITIVE && name == "char"; }
    bool isString() const  { return kind == TypeKind::PRIMITIVE && name == "string"; }
    bool isBool() const    { return kind == TypeKind::PRIMITIVE && name == "bool"; }
    bool isVoid() const    { return kind == TypeKind::VOID_TYPE; }
    bool isError() const   { return kind == TypeKind::ERROR_TYPE; }
    bool isNumeric() const { return isInteger() || isFloat(); }

    // Check if this type can be assigned to 'target' (e.g., Dog is assignable to Animal)
    bool isAssignableTo(const Type* target) const;

    // String representation for error messages (e.g. "int[]", "Animal")
    std::string toString() const;

    // --- Static Factory / Singleton Helpers ---
    static const Type* getInt();
    static const Type* getFloat();
    static const Type* getChar();
    static const Type* getString();
    static const Type* getBool();
    static const Type* getVoid();
    static const Type* getError();

    // Complex Type Factory Helpers
    static const Type* makeArray(const Type* element);
    static const Type* makeFunction(const Type* returnType, std::vector<const Type*> params);
};
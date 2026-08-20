#include "../../include/Semantic/Type.h"

// Singleton instances for built-in primitive types
static const Type TYPE_INT   { TypeKind::PRIMITIVE, "int" };
static const Type TYPE_FLOAT { TypeKind::PRIMITIVE, "float" };
static const Type TYPE_CHAR  { TypeKind::PRIMITIVE, "char" };
static const Type TYPE_STRING{ TypeKind::PRIMITIVE, "string" };
static const Type TYPE_BOOL  { TypeKind::PRIMITIVE, "bool" };
static const Type TYPE_VOID  { TypeKind::VOID_TYPE, "void" };
static const Type TYPE_ERROR { TypeKind::ERROR_TYPE, "<error>" };

const Type* Type::getInt()    { return &TYPE_INT; }
const Type* Type::getFloat()  { return &TYPE_FLOAT; }
const Type* Type::getChar()   { return &TYPE_CHAR; }
const Type* Type::getString() { return &TYPE_STRING; }
const Type* Type::getBool()   { return &TYPE_BOOL; }
const Type* Type::getVoid()   { return &TYPE_VOID; }
const Type* Type::getError()  { return &TYPE_ERROR; }

// Pool to cache complex types (Arrays, Functions) to ensure fast pointer comparisons
static std::vector<std::unique_ptr<Type>> typePool;

const Type* Type::makeArray(const Type* element) {
    auto arrayType = std::make_unique<Type>();
    arrayType->kind = TypeKind::ARRAY;
    arrayType->name = element->name + "[]";
    arrayType->elementType = element;
    
    const Type* ptr = arrayType.get();
    typePool.push_back(std::move(arrayType));
    return ptr;
}

const Type* Type::makeFunction(const Type* returnType, std::vector<const Type*> params) {
    auto funcType = std::make_unique<Type>();
    funcType->kind = TypeKind::FUNCTION;
    funcType->returnType = returnType;
    funcType->paramTypes = std::move(params);

    std::string sig = "(";
    for (size_t i = 0; i < funcType->paramTypes.size(); ++i) {
        sig += funcType->paramTypes[i]->name;
        if (i + 1 < funcType->paramTypes.size()) sig += ", ";
    }
    sig += ") -> " + returnType->name;
    funcType->name = sig;

    const Type* ptr = funcType.get();
    typePool.push_back(std::move(funcType));
    return ptr;
}

bool Type::isAssignableTo(const Type* target) const {
    if (this == target) return true;
    if (this->isError() || target->isError()) return true; // Suppress cascade errors

    // Allow assigning an integer to a float (Implicit widening promotion)
    if (this->isInteger() && target->isFloat()) return true;

    // For Arrays: check element assignability
    if (this->kind == TypeKind::ARRAY && target->kind == TypeKind::ARRAY) {
        return this->elementType->isAssignableTo(target->elementType);
    }

    // For OOP Classes: check inheritance chain (e.g. Dog extends Animal)
    if (this->kind == TypeKind::CLASS && target->kind == TypeKind::CLASS) {
        const Type* current = this->superclass;
        while (current != nullptr) {
            if (current == target) return true;
            current = current->superclass;
        }
    }

    return false;
}

std::string Type::toString() const {
    return name;
}
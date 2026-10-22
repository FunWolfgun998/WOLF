#include "../../include/Semantic/Type.h"

// Singleton instances for built-in primitive types
static const Type TYPE_INT   { TypeKind::PRIMITIVE, "int" };
static const Type TYPE_FLOAT { TypeKind::PRIMITIVE, "float" };
static const Type TYPE_CHAR  { TypeKind::PRIMITIVE, "char" };
static const Type TYPE_STRING{ TypeKind::PRIMITIVE, "string" };
static const Type TYPE_BOOL  { TypeKind::PRIMITIVE, "bool" };
static const Type TYPE_VOID  { TypeKind::VOID_TYPE, "void" };
static const Type TYPE_NULL { TypeKind::NULL_TYPE, "null", true };
static const Type TYPE_ERROR { TypeKind::ERROR_TYPE, "<error>" };

const Type* Type::getInt()    { return &TYPE_INT; }
const Type* Type::getFloat()  { return &TYPE_FLOAT; }
const Type* Type::getChar()   { return &TYPE_CHAR; }
const Type* Type::getString() { return &TYPE_STRING; }
const Type* Type::getBool()   { return &TYPE_BOOL; }
const Type* Type::getVoid()   { return &TYPE_VOID; }
const Type* Type::getNull() { return &TYPE_NULL; }
const Type* Type::getError()  { return &TYPE_ERROR; }

// Pool to cache complex types (Arrays, Functions) to ensure fast pointer comparisons
static std::vector<std::unique_ptr<Type>> typePool;

static std::unordered_map<const Type*, const Type*> arrayTypeCache;
static std::unordered_map<const Type*, const Type*> nullableTypeCache;

const Type* Type::makeFunction(const Type* returnType, std::vector<const Type*> params, size_t minArity) {
    auto funcType = std::make_unique<Type>();
    funcType->kind = TypeKind::FUNCTION;
    funcType->returnType = returnType;
    funcType->paramTypes = std::move(params);
    funcType->minArity = minArity;

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

const Type* Type::makeArray(const Type* element) {
    if (element == nullptr || element->isError()) return Type::getError();

    // Return existing canonical pointer if array of this element type was already created
    auto it = arrayTypeCache.find(element);
    if (it != arrayTypeCache.end()) {
        return it->second;
    }

    auto arrayType = std::make_unique<Type>();
    arrayType->kind = TypeKind::ARRAY;
    arrayType->name = element->name + "[]";
    arrayType->elementType = element;

    const Type* ptr = arrayType.get();
    arrayTypeCache[element] = ptr;
    typePool.push_back(std::move(arrayType));
    return ptr;
}

const Type* Type::makeNullable(const Type* base) {
    if (base == nullptr || base->isNullable || base->isError() || base->isVoid() || base->isNull()) {
        return base;
    }

    // Return existing canonical pointer if nullable version of this type was already created
    auto it = nullableTypeCache.find(base);
    if (it != nullableTypeCache.end()) {
        return it->second;
    }

    auto nullableType = std::make_unique<Type>(*base);
    nullableType->isNullable = true;
    nullableType->name = base->name + "?";

    const Type* ptr = nullableType.get();
    nullableTypeCache[base] = ptr;
    typePool.push_back(std::move(nullableType));
    return ptr;
}


bool Type::isAssignableTo(const Type* target) const {
    // Exact pointer identity match
    if (this == target) return true;

    // Suppress cascade diagnostics if either type is an error sentinel
    if (this->isError() || target->isError()) return true;

    // Void cannot be assigned to any variable
    if (this->isVoid() || target->isVoid()) return false;

    // 'null' can only be assigned to explicitly nullable targets (T?)
    if (this->isNull()) {
        return target->isNullable;
    }

    // A nullable source (T?) cannot be assigned to a non-nullable target (T)
    if (this->isNullable && !target->isNullable) {
        return false;
    }

    // Non-nullable source T is assignable to nullable target T? (e.g. int -> int?, Dog -> Dog?)
    if (!this->isNullable && target->isNullable) {
        if (this->name + "?" == target->name) return true;
    }

    // Implicit numeric widening: int -> float
    if (this->isInteger() && (target->isFloat() || target->name == "float?")) return true;

    // Recursive array element assignability (e.g. int[] -> int?[] or int[] -> int[])
    if (this->kind == TypeKind::ARRAY && target->kind == TypeKind::ARRAY) {
        // Un letterale array vuoto (void[]) e' assegnabile a qualsiasi array target (T[], T?[], T[][])
        if (this->elementType->isVoid()) return true;
        return this->elementType->isAssignableTo(target->elementType);
    }

    // OOP inheritance hierarchy traversal (e.g. Dog -> Animal or Dog -> Animal?)
    if (this->kind == TypeKind::CLASS && target->kind == TypeKind::CLASS) {
        const Type* current = this->superclass;
        std::string targetBaseName = target->isNullable ? target->name.substr(0, target->name.size() - 1) : target->name;
        while (current != nullptr) {
            if (current == target || current->name == targetBaseName) return true;
            current = current->superclass;
        }
    }

    // Match base type names
    if (this->name == target->name) return true;

    return false;
}

std::string Type::toString() const {
    return name;
}
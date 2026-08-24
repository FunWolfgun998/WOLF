#pragma once

#include <string>

enum class ErrorCode {
    E0001_UNDECLARED_VARIABLE,
    E0002_TYPE_MISMATCH,
    E0003_INVALID_ASSIGNMENT_TARGET,
    E0004_DUPLICATE_DECLARATION,
    E0005_BREAK_OUTSIDE_LOOP,
    E0006_CONTINUE_OUTSIDE_LOOP,
    E0007_RETURN_OUTSIDE_FUNCTION,
    E0008_RETURN_TYPE_MISMATCH,
    E0009_MISSING_RETURN_VALUE,
    E0010_RETURN_IN_VOID_FUNCTION,
    E0011_THIS_OUTSIDE_CLASS,
    E0012_PRIVATE_MEMBER_ACCESS,
    E0013_MEMBER_NOT_FOUND,
    E0014_INVALID_SUBSCRIPT_TARGET,
    E0015_INVALID_SUBSCRIPT_INDEX,
    E0016_ARRAY_ELEMENT_TYPE_MISMATCH,
    E0017_ARGUMENT_COUNT_MISMATCH,
    E0018_ARGUMENT_TYPE_MISMATCH,
    E0019_NON_BOOLEAN_CONDITION,
    E0020_INVALID_OPERATOR_OPERANDS,
    E0021_INVALID_RANGE_BOUNDS,
    E0022_INCOMPATIBLE_TERNARY_BRANCHES,
    E0023_DEFAULT_PARAM_ORDER,
    E0024_UNKNOWN_TYPE_NAME,
    E0025_NOT_CALLABLE
};

// Returns the textual error code (e.g. "E0001")
inline std::string errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::E0001_UNDECLARED_VARIABLE:          return "E0001";
        case ErrorCode::E0002_TYPE_MISMATCH:                 return "E0002";
        case ErrorCode::E0003_INVALID_ASSIGNMENT_TARGET:     return "E0003";
        case ErrorCode::E0004_DUPLICATE_DECLARATION:         return "E0004";
        case ErrorCode::E0005_BREAK_OUTSIDE_LOOP:            return "E0005";
        case ErrorCode::E0006_CONTINUE_OUTSIDE_LOOP:         return "E0006";
        case ErrorCode::E0007_RETURN_OUTSIDE_FUNCTION:       return "E0007";
        case ErrorCode::E0008_RETURN_TYPE_MISMATCH:          return "E0008";
        case ErrorCode::E0009_MISSING_RETURN_VALUE:          return "E0009";
        case ErrorCode::E0010_RETURN_IN_VOID_FUNCTION:       return "E0010";
        case ErrorCode::E0011_THIS_OUTSIDE_CLASS:            return "E0011";
        case ErrorCode::E0012_PRIVATE_MEMBER_ACCESS:         return "E0012";
        case ErrorCode::E0013_MEMBER_NOT_FOUND:              return "E0013";
        case ErrorCode::E0014_INVALID_SUBSCRIPT_TARGET:      return "E0014";
        case ErrorCode::E0015_INVALID_SUBSCRIPT_INDEX:       return "E0015";
        case ErrorCode::E0016_ARRAY_ELEMENT_TYPE_MISMATCH:   return "E0016";
        case ErrorCode::E0017_ARGUMENT_COUNT_MISMATCH:       return "E0017";
        case ErrorCode::E0018_ARGUMENT_TYPE_MISMATCH:        return "E0018";
        case ErrorCode::E0019_NON_BOOLEAN_CONDITION:         return "E0019";
        case ErrorCode::E0020_INVALID_OPERATOR_OPERANDS:     return "E0020";
        case ErrorCode::E0021_INVALID_RANGE_BOUNDS:          return "E0021";
        case ErrorCode::E0022_INCOMPATIBLE_TERNARY_BRANCHES: return "E0022";
        case ErrorCode::E0023_DEFAULT_PARAM_ORDER:           return "E0023";
        case ErrorCode::E0024_UNKNOWN_TYPE_NAME:             return "E0024";
        case ErrorCode::E0025_NOT_CALLABLE:                  return "E0025";
        default:                                             return "E9999";
    }
}
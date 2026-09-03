#pragma once

#include <string>

// Comprehensive catalog of all compiler diagnostic messages
enum class DiagnosticCode {
    // --- ERRORS (E): Breaking semantic violations ---

    // Variables & Scopes
    E0001_UNDECLARED_VARIABLE,
    E0002_UNINITIALIZED_VARIABLE,
    E0003_CANNOT_ASSIGN_CONSTANT,
    E0004_DUPLICATE_DECLARATION,
    E0005_INVALID_ASSIGNMENT_TARGET,

    // Types & Operations
    E0006_TYPE_MISMATCH,
    E0007_UNKNOWN_TYPE_NAME,
    E0008_INTEGER_OVERFLOW,
    E0009_NON_BOOLEAN_CONDITION,
    E0010_INVALID_OPERATOR_OPERANDS,
    E0011_INCOMPATIBLE_TERNARY_BRANCHES,
    E0012_INVALID_RANGE_BOUNDS,

    // Null Safety
    E0013_NULL_ASSIGNED_TO_NON_NULLABLE,
    E0014_NULLABLE_MEMBER_DEREFERENCE,
    E0015_NULLABLE_ARRAY_SUBSCRIPT,

    // Arrays
    E0016_ARRAY_ELEMENT_TYPE_MISMATCH,
    E0017_INVALID_SUBSCRIPT_TARGET,
    E0018_INVALID_SUBSCRIPT_INDEX,

    // Control Flow
    E0019_BREAK_OUTSIDE_LOOP,
    E0020_CONTINUE_OUTSIDE_LOOP,
    E0021_RETURN_OUTSIDE_FUNCTION,
    E0022_RETURN_TYPE_MISMATCH,
    E0023_MISSING_RETURN_VALUE,
    E0024_RETURN_IN_VOID_FUNCTION,
    E0025_NOT_ALL_PATHS_RETURN,

    // OOP, Classes, Structs & Methods
    E0026_THIS_OUTSIDE_CLASS,
    E0027_THIS_IN_STATIC_METHOD,
    E0028_PRIVATE_MEMBER_ACCESS,
    E0029_PROTECTED_MEMBER_ACCESS,
    E0030_MEMBER_NOT_FOUND,
    E0031_NOT_CALLABLE,
    E0032_ARGUMENT_COUNT_MISMATCH,
    E0033_ARGUMENT_TYPE_MISMATCH,
    E0034_DEFAULT_PARAM_ORDER,
    E0035_CYCLIC_INHERITANCE,
    E0036_CANNOT_INSTANTIATE_ABSTRACT_CLASS,
    E0037_UNIMPLEMENTED_ABSTRACT_METHOD,
    E0038_INVALID_OVERRIDE,
    E0039_CANNOT_EXTEND_FINAL_CLASS,
    E0040_DUPLICATE_CLASS_MEMBER,
    E0041_VOID_VARIABLE_DECLARATION,

    // --- WARNINGS (W): Non-breaking code smells ---
    W0001_UNUSED_VARIABLE,
    W0002_UNUSED_PARAMETER,
    W0003_UNREACHABLE_CODE,
    W0004_VARIABLE_SHADOWING,
    W0005_NUMERIC_PRECISION_LOSS,
    W0006_UNUSED_IMPORT,
    W0007_EMPTY_BLOCK,
    W0008_STATIC_CALLED_ON_INSTANCE,

    // --- NOTES (N): Contextual auxiliary information ---
    N0001_PREVIOUS_DECLARATION,
    N0002_HELP_SUGGESTION,
    N0003_TYPE_INFERRED_HERE
};

// Formats diagnostic code into textual ID (e.g. "E0001", "W0001")
inline std::string diagnosticCodeToString(DiagnosticCode code) {
    switch (code) {
        // Errors
        case DiagnosticCode::E0001_UNDECLARED_VARIABLE:          return "E0001";
        case DiagnosticCode::E0002_UNINITIALIZED_VARIABLE:        return "E0002";
        case DiagnosticCode::E0003_CANNOT_ASSIGN_CONSTANT:       return "E0003";
        case DiagnosticCode::E0004_DUPLICATE_DECLARATION:         return "E0004";
        case DiagnosticCode::E0005_INVALID_ASSIGNMENT_TARGET:     return "E0005";
        case DiagnosticCode::E0006_TYPE_MISMATCH:                 return "E0006";
        case DiagnosticCode::E0007_UNKNOWN_TYPE_NAME:             return "E0007";
        case DiagnosticCode::E0008_INTEGER_OVERFLOW:              return "E0008";
        case DiagnosticCode::E0009_NON_BOOLEAN_CONDITION:         return "E0009";
        case DiagnosticCode::E0010_INVALID_OPERATOR_OPERANDS:     return "E0010";
        case DiagnosticCode::E0011_INCOMPATIBLE_TERNARY_BRANCHES: return "E0011";
        case DiagnosticCode::E0012_INVALID_RANGE_BOUNDS:          return "E0012";
        case DiagnosticCode::E0013_NULL_ASSIGNED_TO_NON_NULLABLE: return "E0013";
        case DiagnosticCode::E0014_NULLABLE_MEMBER_DEREFERENCE:   return "E0014";
        case DiagnosticCode::E0015_NULLABLE_ARRAY_SUBSCRIPT:      return "E0015";
        case DiagnosticCode::E0016_ARRAY_ELEMENT_TYPE_MISMATCH:   return "E0016";
        case DiagnosticCode::E0017_INVALID_SUBSCRIPT_TARGET:      return "E0017";
        case DiagnosticCode::E0018_INVALID_SUBSCRIPT_INDEX:       return "E0018";
        case DiagnosticCode::E0019_BREAK_OUTSIDE_LOOP:            return "E0019";
        case DiagnosticCode::E0020_CONTINUE_OUTSIDE_LOOP:         return "E0020";
        case DiagnosticCode::E0021_RETURN_OUTSIDE_FUNCTION:       return "E0021";
        case DiagnosticCode::E0022_RETURN_TYPE_MISMATCH:          return "E0022";
        case DiagnosticCode::E0023_MISSING_RETURN_VALUE:          return "E0023";
        case DiagnosticCode::E0024_RETURN_IN_VOID_FUNCTION:       return "E0024";
        case DiagnosticCode::E0025_NOT_ALL_PATHS_RETURN:          return "E0025";
        case DiagnosticCode::E0026_THIS_OUTSIDE_CLASS:            return "E0026";
        case DiagnosticCode::E0027_THIS_IN_STATIC_METHOD:         return "E0027";
        case DiagnosticCode::E0028_PRIVATE_MEMBER_ACCESS:         return "E0028";
        case DiagnosticCode::E0029_PROTECTED_MEMBER_ACCESS:       return "E0029";
        case DiagnosticCode::E0030_MEMBER_NOT_FOUND:              return "E0030";
        case DiagnosticCode::E0031_NOT_CALLABLE:                  return "E0031";
        case DiagnosticCode::E0032_ARGUMENT_COUNT_MISMATCH:       return "E0032";
        case DiagnosticCode::E0033_ARGUMENT_TYPE_MISMATCH:        return "E0033";
        case DiagnosticCode::E0034_DEFAULT_PARAM_ORDER:           return "E0034";
        case DiagnosticCode::E0035_CYCLIC_INHERITANCE:            return "E0035";
        case DiagnosticCode::E0036_CANNOT_INSTANTIATE_ABSTRACT_CLASS: return "E0036";
        case DiagnosticCode::E0037_UNIMPLEMENTED_ABSTRACT_METHOD: return "E0037";
        case DiagnosticCode::E0038_INVALID_OVERRIDE:              return "E0038";
        case DiagnosticCode::E0039_CANNOT_EXTEND_FINAL_CLASS:     return "E0039";
        case DiagnosticCode::E0040_DUPLICATE_CLASS_MEMBER:        return "E0040";
        case DiagnosticCode::E0041_VOID_VARIABLE_DECLARATION:     return "E0041";

        // Warnings
        case DiagnosticCode::W0001_UNUSED_VARIABLE:               return "W0001";
        case DiagnosticCode::W0002_UNUSED_PARAMETER:              return "W0002";
        case DiagnosticCode::W0003_UNREACHABLE_CODE:              return "W0003";
        case DiagnosticCode::W0004_VARIABLE_SHADOWING:            return "W0004";
        case DiagnosticCode::W0005_NUMERIC_PRECISION_LOSS:        return "W0005";
        case DiagnosticCode::W0006_UNUSED_IMPORT:                 return "W0006";
        case DiagnosticCode::W0007_EMPTY_BLOCK:                   return "W0007";
        case DiagnosticCode::W0008_STATIC_CALLED_ON_INSTANCE:     return "W0008";

        // Notes
        case DiagnosticCode::N0001_PREVIOUS_DECLARATION:          return "N0001";
        case DiagnosticCode::N0002_HELP_SUGGESTION:               return "N0002";
        case DiagnosticCode::N0003_TYPE_INFERRED_HERE:            return "N0003";

        default:                                                  return "D9999";
    }
}
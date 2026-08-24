#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include "../Lexer/Token.h"
#include "ErrorCode.h"

// Structured record representing a single diagnostic issue
struct Diagnostic {
    ErrorCode code;
    Token token;             // Holds coordinates (line, column, length)
    std::string message;     // Main explanation of the error
    std::string hint;        // Optional suggestion (e.g. "did you mean '...'?")
};

class DiagnosticEngine {
private:
    std::string sourceCode;
    std::string filename;
    std::vector<Diagnostic> diagnostics;

    // Slices a specific 1-based line of code from sourceCode on-demand
    std::string extractLine(int lineNumber) const;

    // Computes Levenshtein edit distance between two strings
    size_t editDistance(const std::string& a, const std::string& b) const;

public:
    DiagnosticEngine() = default;

    // Stores source code reference for on-demand snippet extraction
    void setSource(std::string file, std::string source) {
        filename = std::move(file);
        sourceCode = std::move(source);
    }

    // Records a new diagnostic error
    void report(ErrorCode code, const Token& token, std::string message, std::string hint = "");

    bool hasErrors() const { return !diagnostics.empty(); }
    size_t errorCount() const { return diagnostics.size(); }
    const std::vector<Diagnostic>& getDiagnostics() const { return diagnostics; }
    void clear() { diagnostics.clear(); }

    // Searches candidates and returns the closest match if edit distance <= 2
    std::string findClosestMatch(const std::string& target, const std::vector<std::string>& candidates) const;

    // Formats and prints all diagnostics (Rust-style by default, or single-line for IDEs)
    void print(std::ostream& out, bool ideMode = false) const;
};
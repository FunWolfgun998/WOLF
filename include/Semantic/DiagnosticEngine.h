#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstddef>
#include "../Lexer/Token.h"
#include "DiagnosticCode.h"

// Severity level of a diagnostic message
enum class DiagnosticSeverity {
    ERROR,
    WARNING,
    NOTE,
    HELP
};

// Represents a 1-based source coordinate range with column length
struct SourceSpan {
    int line = 1;
    int column = 1;
    size_t length = 1;

    SourceSpan() = default;
    SourceSpan(int l, int c, size_t len) : line(l), column(c), length(len) {}
    SourceSpan(const Token& tok)
        : line(tok.line), column(tok.column), length(std::max((size_t)1, tok.lexeme.length())) {}
};

// An annotated region of source code with an inline message attached to the carets
struct SpanLabel {
    SourceSpan span;
    std::string label;
    bool isPrimary = true;

    SpanLabel() = default;
    SpanLabel(SourceSpan s, std::string lbl = "", bool primary = true)
        : span(s), label(std::move(lbl)), isPrimary(primary) {}
    SpanLabel(const Token& tok, std::string lbl = "", bool primary = true)
        : span(tok), label(std::move(lbl)), isPrimary(primary) {}
};

// Structured diagnostic record supporting multiple spans and suggestions
struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::ERROR;
    DiagnosticCode code = DiagnosticCode::E0001_UNDECLARED_VARIABLE;
    std::string message;
    std::vector<SpanLabel> labels;
    std::string hint;
};

class DiagnosticEngine {
private:
    // Full raw source text in memory for on-demand snippet slicing
    std::string sourceCode;

    // Path or name of the input source file for diagnostic location headers
    std::string filename;

    // Accumulated list of all reported diagnostics during compilation
    std::vector<Diagnostic> diagnostics;

    // Byte offsets of line starts in sourceCode for O(1) text extraction
    std::vector<size_t> lineOffsets;

    // Scans sourceCode once to precompute byte offsets for every line
    void buildLineIndex();

    // Extracts a single 1-based line of code and expands tabs to spaces
    std::string extractLine(int lineNumber) const;

    // Computes Levenshtein edit distance between two strings
    size_t editDistance(const std::string& a, const std::string& b) const;

public:
    DiagnosticEngine() = default;

    // Configures the source text and precomputes the line index
    void setSource(std::string file, std::string source);

    // Records a single-span diagnostic error
    void report(DiagnosticCode code, const SourceSpan& span, std::string message, std::string hint = "");

    // Records a multi-span diagnostic error spanning multiple source locations
    void reportMultiSpan(DiagnosticCode code, std::string message, const std::vector<SpanLabel>& labels, std::string hint = "");

    // Records a warning diagnostic (does not prevent execution)
    void reportWarning(DiagnosticCode code, const SourceSpan& span, std::string message, std::string hint = "");

    // Returns true if any diagnostic has ERROR severity
    bool hasErrors() const;

    // Returns the total number of accumulated diagnostics
    size_t errorCount() const { return diagnostics.size(); }

    // Accessor for the list of recorded diagnostics
    const std::vector<Diagnostic>& getDiagnostics() const { return diagnostics; }

    // Clears all accumulated diagnostics
    void clear() { diagnostics.clear(); }

    // Finds the closest candidate identifier with edit distance <= 2 for typo suggestions
    std::string findClosestMatch(const std::string& target, const std::vector<std::string>& candidates) const;

    // Formats and renders all diagnostics to the output stream (Rust-style or single-line IDE mode)
    void print(std::ostream& out, bool ideMode = false) const;
};
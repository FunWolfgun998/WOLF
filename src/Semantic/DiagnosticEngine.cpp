#include "../../include/Semantic/DiagnosticEngine.h"
#include <sstream>

void DiagnosticEngine::setSource(std::string file, std::string source) {
    filename = std::move(file);
    sourceCode = std::move(source);
    buildLineIndex();
}

void DiagnosticEngine::buildLineIndex() {
    lineOffsets.clear();
    // Line 1 starts at byte offset 0
    lineOffsets.push_back(0);

    // Record byte index immediately following every newline
    for (size_t i = 0; i < sourceCode.size(); ++i) {
        if (sourceCode[i] == '\n') {
            lineOffsets.push_back(i + 1);
        }
    }
}

std::string DiagnosticEngine::extractLine(int lineNumber) const {
    if (lineNumber <= 0 || (size_t)lineNumber > lineOffsets.size()) {
        return "";
    }

    size_t start = lineOffsets[lineNumber - 1];
    size_t end = (size_t)lineNumber < lineOffsets.size() ? lineOffsets[lineNumber] - 1 : sourceCode.size();

    // Prevent out-of-bounds slicing on empty trailing lines
    if (start >= sourceCode.size()) return "";

    std::string raw = sourceCode.substr(start, end - start);

    // Strip carriage return on CRLF files
    if (!raw.empty() && raw.back() == '\r') {
        raw.pop_back();
    }

    // Expand tabs to 4 spaces to synchronize terminal column calculations
    std::string expanded;
    for (char c : raw) {
        if (c == '\t') expanded += "    ";
        else expanded += c;
    }
    return expanded;
}
size_t DiagnosticEngine::editDistance(const std::string& s1, const std::string& s2) const {
    const size_t m = s1.size();
    const size_t n = s2.size();

    // Allocate dynamic programming table (m+1) x (n+1)
    std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1));

    // Base cases: distance between empty string and non-empty prefixes
    for (size_t i = 0; i <= m; ++i) dp[i][0] = i; // Deleting all characters from s1 prefix
    for (size_t j = 0; j <= n; ++j) dp[0][j] = j; // Inserting all characters to form s2 prefix

    // Populate DP matrix iteratively
    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            if (s1[i - 1] == s2[j - 1]) {
                // Characters match: zero cost transition from previous diagonal state
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                // Compute minimum edit cost among deletion, insertion, and substitution
                size_t deleteCost     = dp[i - 1][j];
                size_t insertCost     = dp[i][j - 1];
                size_t substituteCost = dp[i - 1][j - 1];

                dp[i][j] = 1 + std::min({deleteCost, insertCost, substituteCost});
            }
        }
    }

    // Bottom-right cell contains the minimal total transformation distance
    return dp[m][n];
}

std::string DiagnosticEngine::findClosestMatch(const std::string& target, const std::vector<std::string>& candidates) const {
    std::string bestMatch = "";
    size_t minDistance = 3; // Maximum acceptable edit distance threshold (1 or 2 edits allowed)

    for (const auto& candidate : candidates) {
        // Skip identical words
        if (candidate == target) continue;

        // Skip candidates whose length difference mathematically guarantees exceeding the threshold
        if (std::abs((int)candidate.length() - (int)target.length()) > 2) continue;

        // Compute edit distance and track the closest candidate
        size_t dist = editDistance(target, candidate);
        if (dist < minDistance) {
            minDistance = dist;
            bestMatch = candidate;
        }
    }

    return bestMatch;
}

void DiagnosticEngine::report(DiagnosticCode code, const SourceSpan& span, std::string message, std::string hint) {
    Diagnostic diag;
    diag.severity = DiagnosticSeverity::ERROR;
    diag.code = code;
    diag.message = std::move(message);
    diag.labels.push_back(SpanLabel{span, "", true});
    diag.hint = std::move(hint);
    diagnostics.push_back(std::move(diag));
}

void DiagnosticEngine::reportMultiSpan(DiagnosticCode code, std::string message, const std::vector<SpanLabel>& labels, std::string hint) {
    Diagnostic diag;
    diag.severity = DiagnosticSeverity::ERROR;
    diag.code = code;
    diag.message = std::move(message);
    diag.labels = labels;
    diag.hint = std::move(hint);
    diagnostics.push_back(std::move(diag));
}

void DiagnosticEngine::reportWarning(DiagnosticCode code, const SourceSpan& span, std::string message, std::string hint) {
    Diagnostic diag;
    diag.severity = DiagnosticSeverity::WARNING;
    diag.code = code;
    diag.message = std::move(message);
    diag.labels.push_back(SpanLabel{span, "", true});
    diag.hint = std::move(hint);
    diagnostics.push_back(std::move(diag));
}

bool DiagnosticEngine::hasErrors() const {
    for (const auto& diag : diagnostics) {
        if (diag.severity == DiagnosticSeverity::ERROR) return true;
    }
    return false;
}

void DiagnosticEngine::print(std::ostream& out, bool ideMode) const {
    for (const auto& diag : diagnostics) {
        std::string codeStr = diagnosticCodeToString(diag.code);

        // Identify primary label coordinates for the header navigation line
        int primaryLine = diag.labels.empty() ? 1 : diag.labels[0].span.line;
        int primaryCol = diag.labels.empty() ? 1 : diag.labels[0].span.column;
        for (const auto& lbl : diag.labels) {
            if (lbl.isPrimary) {
                primaryLine = lbl.span.line;
                primaryCol = lbl.span.column;
                break;
            }
        }

        // Emit single-line output compatible with IDE problem matchers
        if (ideMode) {
            std::string sevStr = (diag.severity == DiagnosticSeverity::WARNING) ? "warning" : "error";
            out << filename << ":" << primaryLine << ":" << primaryCol
                << ": " << sevStr << "[" << codeStr << "]: " << diag.message << "\n";
            continue;
        }

        // Determine maximum line number to synchronize gutter column width
        int maxLine = primaryLine;
        for (const auto& lbl : diag.labels) {
            maxLine = std::max(maxLine, lbl.span.line);
        }
        size_t margin = std::max((size_t)2, std::to_string(maxLine).length());
        std::string emptyGutter(margin + 2, ' ');

        // Print header and source location path
        std::string sevHeader = (diag.severity == DiagnosticSeverity::WARNING) ? "\033[1;33mwarning" : "\033[1;31merror";
        out << sevHeader << "[" << codeStr << "]\033[0m: \033[1m" << diag.message << "\033[0m\n";
        out << "  \033[1;34m-->\033[0m " << filename << ":" << primaryLine << ":" << primaryCol << "\n";
        out << "\033[1;34m" << emptyGutter << "|\033[0m\n";

        // Sort labels in ascending order by line number
        std::vector<SpanLabel> sortedLabels = diag.labels;
        std::sort(sortedLabels.begin(), sortedLabels.end(), [](const SpanLabel& a, const SpanLabel& b) {
            return a.span.line < b.span.line;
        });

        int prevLine = -1;
        for (const auto& lbl : sortedLabels) {
            // Print ellipsis when skipping unannotated lines between spans
            if (prevLine != -1 && lbl.span.line > prevLine + 1) {
                out << "\033[1;34m" << emptyGutter << "...\033[0m\n";
            }

            std::string lineText = extractLine(lbl.span.line);
            std::string lineNumStr = std::to_string(lbl.span.line);
            std::string lineGutter = std::string(margin - lineNumStr.length() + 1, ' ') + lineNumStr + " ";

            // Print source line
            out << "\033[1;34m" << lineGutter << "|\033[0m " << lineText << "\n";

            // Print caret/dash markers aligned with column offset
            out << "\033[1;34m" << emptyGutter << "|\033[0m ";
            size_t col = lbl.span.column > 0 ? (size_t)lbl.span.column : 1;
            for (size_t i = 1; i < col; ++i) {
                out << " ";
            }

            // Primary labels use red '^', secondary context labels use cyan '-'
            if (lbl.isPrimary) {
                out << "\033[1;31m";
                for (size_t i = 0; i < lbl.span.length; ++i) out << "^";
                if (!lbl.label.empty()) out << " " << lbl.label;
                out << "\033[0m\n";
            } else {
                out << "\033[1;36m";
                for (size_t i = 0; i < lbl.span.length; ++i) out << "-";
                if (!lbl.label.empty()) out << " " << lbl.label;
                out << "\033[0m\n";
            }
            prevLine = lbl.span.line;
        }

        // Print optional help message
        if (!diag.hint.empty()) {
            out << "\033[1;34m" << emptyGutter << "=\033[0m \033[1;36mhelp: " << diag.hint << "\n";
        }
        out << "\n";
    }
}
#include "../../include/Semantic/DiagnosticEngine.h"
#include <sstream>

void DiagnosticEngine::report(ErrorCode code, const SourceSpan& span, std::string message, std::string hint) {
    diagnostics.push_back(Diagnostic{code, span, std::move(message), std::move(hint)});
}

std::string DiagnosticEngine::extractLine(int lineNumber) const {
    if (sourceCode.empty() || lineNumber <= 0) return "";

    std::istringstream stream(sourceCode);
    std::string lineText;
    int currentLine = 1;

    while (std::getline(stream, lineText)) {
        if (currentLine == lineNumber) {
            if (!lineText.empty() && lineText.back() == '\r') {
                lineText.pop_back();
            }
            // Expand tabs to 4 spaces for terminal column synchronization
            std::string expanded;
            for (char c : lineText) {
                if (c == '\t') expanded += "    ";
                else expanded += c;
            }
            return expanded;
        }
        currentLine++;
    }
    return "";
}

size_t DiagnosticEngine::editDistance(const std::string& s1, const std::string& s2) const {
    const size_t m = s1.size();
    const size_t n = s2.size();
    std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1));

    for (size_t i = 0; i <= m; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= n; ++j) dp[0][j] = j;

    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            if (s1[i - 1] == s2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }
    return dp[m][n];
}

std::string DiagnosticEngine::findClosestMatch(const std::string& target, const std::vector<std::string>& candidates) const {
    std::string bestMatch = "";
    size_t minDistance = 3; // Strictly require distance <= 2

    for (const auto& candidate : candidates) {
        // Skip identical strings or candidates with massive length difference
        if (candidate == target) continue;
        if (std::abs((int)candidate.length() - (int)target.length()) > 2) continue;

        size_t dist = editDistance(target, candidate);
        if (dist < minDistance) {
            minDistance = dist;
            bestMatch = candidate;
        }
    }
    return bestMatch;
}

void DiagnosticEngine::print(std::ostream& out, bool ideMode) const {
    for (const auto& diag : diagnostics) {
        std::string codeStr = errorCodeToString(diag.code);

        if (ideMode) {
            out << filename << ":" << diag.span.line << ":" << diag.span.column
                << ": error[" << codeStr << "]: " << diag.message << "\n";
            continue;
        }

        // 1. Error header in Bold Red
        out << "\033[1;31merror[" << codeStr << "]\033[0m: \033[1m" << diag.message << "\033[0m\n";

        // 2. File and location in Blue
        out << "  \033[1;34m-->\033[0m " << filename << ":" << diag.span.line << ":" << diag.span.column << "\n";

        std::string lineText = extractLine(diag.span.line);
        std::string lineNumStr = std::to_string(diag.span.line);
        size_t margin = std::max((size_t)2, lineNumStr.length());

        std::string emptyGutter(margin + 2, ' ');
        std::string lineGutter = std::string(margin - lineNumStr.length() + 1, ' ') + lineNumStr + " ";

        // 3. Code snippet framing
        out << "\033[1;34m" << emptyGutter << "|\033[0m\n";
        out << "\033[1;34m" << lineGutter << "|\033[0m " << lineText << "\n";
        out << "\033[1;34m" << emptyGutter << "|\033[0m ";

        // 4. Caret alignment under the exact span column range
        size_t col = diag.span.column > 0 ? (size_t)diag.span.column : 1;
        for (size_t i = 1; i < col; ++i) {
            out << " ";
        }

        out << "\033[1;31m";
        for (size_t i = 0; i < diag.span.length; ++i) {
            out << "^";
        }
        out << "\033[0m\n";

        // 5. Help / Hint line in bright Cyan
        if (!diag.hint.empty()) {
            out << "\033[1;34m" << emptyGutter << "|\033[0m \033[1;34m" << "------ help: " << diag.hint << "\n";
        }
        out << "\n";
    }
}
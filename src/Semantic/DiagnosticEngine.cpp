#include "../../include/Semantic/DiagnosticEngine.h"
#include <sstream>
#include <iomanip>

void DiagnosticEngine::report(ErrorCode code, const Token& token, std::string message, std::string hint) {
    diagnostics.push_back(Diagnostic{code, token, std::move(message), std::move(hint)});
}

std::string DiagnosticEngine::extractLine(int lineNumber) const {
    if (sourceCode.empty() || lineNumber <= 0) return "";

    std::istringstream stream(sourceCode);
    std::string lineText;
    int currentLine = 1;

    while (std::getline(stream, lineText)) {
        if (currentLine == lineNumber) {
            // Strip trailing carriage return character on CRLF line endings
            if (!lineText.empty() && lineText.back() == '\r') {
                lineText.pop_back();
            }
            // Expand tabs to 4 spaces to ensure visual column synchronization
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
    size_t minDistance = 3;

    for (const auto& candidate : candidates) {
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
            out << filename << ":" << diag.token.line << ":" << diag.token.column
                << ": error[" << codeStr << "]: " << diag.message << "\n";
            continue;
        }

        // Header and location path
        out << "\033[1;31merror[" << codeStr << "]\033[0m: \033[1m" << diag.message << "\033[0m\n";
        out << "  \033[1;34m-->\033[0m " << filename << ":" << diag.token.line << ":" << diag.token.column << "\n";

        std::string lineText = extractLine(diag.token.line);
        std::string lineNumStr = std::to_string(diag.token.line);
        size_t margin = std::max((size_t)2, lineNumStr.length());

        // Left gutter spacing perfectly synchronized with line number width
        std::string emptyGutter(margin + 2, ' ');
        std::string lineGutter = std::string(margin - lineNumStr.length() + 1, ' ') + lineNumStr + " ";

        // Top empty bar
        out << "\033[1;34m" << emptyGutter << "|\033[0m\n";

        // Source code line
        out << "\033[1;34m" << lineGutter << "|\033[0m " << lineText << "\n";

        // Carets line: align exactly (column - 1 spaces after the bar)
        out << "\033[1;34m" << emptyGutter << "|\033[0m ";

        size_t col = diag.token.column > 0 ? (size_t)diag.token.column : 1;
        for (size_t i = 1; i < col; ++i) {
            out << " ";
        }

        size_t caretCount = std::max((size_t)1, diag.token.lexeme.length());
        out << "\033[1;31m";
        for (size_t i = 0; i < caretCount; ++i) {
            out << "^";
        }
        out << "\033[0m\n";

        // Help hint
        if (!diag.hint.empty()) {
            out << "\033[1;34m" << emptyGutter << "=\033[0m \033[1;36mhelp:\033[0m " << diag.hint << "\n";
        }
        out << "\n";
    }
}
#include "../../include/Lexer/Lexer.h"
#include <iostream>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <cstdlib>

// Static compile-time hash function (FNV-1a algorithm)
// Used for extremely fast O(1) keyword matching in switch statements.
constexpr uint32_t hash(const char* str, uint32_t h = 0x811c9dc5) {
    return !*str ? h : hash(str + 1, (h ^ *str) * 0x01000193);
}

Lexer::Lexer(std::string source) : source(std::move(source)) {
    // Indentation always starts at level 0 (base level)
    indentStack.push_back(0);
}

// --- Primitive Operations ---

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    column++;
    return source[current++];
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Lexer::peekNext(int n) const {
    if (current + n >= source.length()) return '\0';
    return source[current + n];
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    current++;
    column++;
    return true;
}

bool Lexer::isAtEnd() const {
    return current >= source.length();
}

// Error Handling

void Lexer::manageError(const std::string& message) {
    // Extract the exact piece of code that caused the error
    std::string text = source.substr(start, current - start);
    if (text.empty() && isAtEnd()) text = "EOF"; // Failsafe for end of file

    // Print formatted error to standard error
    std::cerr << "[Lexer Error] Line " << line
              << ", Col " << startColumn
              << ": " << message
              << " -> '" << text << "'" << std::endl;

    // Add the ERROR token to the stream so the Parser is aware of it
    tokens.emplace_back(TokenType::ERROR, text, line, startColumn);
}

void Lexer::recoverFromError() {
    /*
        Panic Mode Recovery: Consume characters until we find a reliable
        synchronization point (like whitespace or structural punctuation).
    */
    while (!isAtEnd()) {
        char c = peek();
        if (isspace(c) || strchr("(){},;", c)) {
            break;
        }
        advance();
    }
}

// --- Main Scanning Loop ---

std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        start = current;
        startColumn = column;
        scanToken();
    }

    // Auto-emit DEDENT tokens at the end of the file to close open blocks
    while (indentStack.back() > 0) {
        indentStack.pop_back();
        tokens.emplace_back(TokenType::DEDENT, "DEDENT", line, column);
    }

    tokens.emplace_back(TokenType::END_OF_FILE, "EOF", line, column);
    return tokens;
}

void Lexer::scanToken() {
    char c = advance();

    switch (c) {
        // Single character tokens
        case '(': addToken(TokenType::L_PAREN); break;
        case ')': addToken(TokenType::R_PAREN); break;
        case ',': addToken(TokenType::COMMA); break;
        case ':': addToken(TokenType::COLON); break;
        case '[': addToken(TokenType::L_BRACKET); break;
        case ']': addToken(TokenType::R_BRACKET); break;
        case '~': addToken(TokenType::OP_TILDE); break;
        case '?': addToken(TokenType::OP_QUESTION); break;

        case '+':
            if (match('+')) addToken(TokenType::OP_INC); // ++
            else if (match('=')) addToken(TokenType::OP_PLUS_ASS); // +=
            else addToken(TokenType::OP_PLUS); // +
            break;

        case '-':
            if (match('-')) addToken(TokenType::OP_DEC); // --
            else if (match('>')) addToken(TokenType::ARROW); // ->
            else if (match('=')) addToken(TokenType::OP_MIN_ASS); // -=
            else addToken(TokenType::OP_MINUS); // -
            break;

        // Maximal Munch matching for 2-character and 3-character operators
        case '*': match('=') ? addToken(TokenType::OP_MUL_ASS) : addToken(TokenType::OP_STAR); break;
        case '%': match('=') ? addToken(TokenType::OP_MOD_ASS) : addToken(TokenType::OP_MOD); break;
        case '^': match('=') ? addToken(TokenType::OP_XOR_ASS) : addToken(TokenType::OP_XOR); break;

        case '=':
            match('=') ? addToken(TokenType::OP_EQ_EQ) : addToken(TokenType::OP_ASSIGN);
            break;

        case '!':
            match('=') ? addToken(TokenType::OP_NOT_EQ) : addToken(TokenType::OP_NOT);
            break;

        // 3-level operators (e.g. <, <=, <<, <<=)
        case '<':
            if (match('<')) {
                if (match('=')) addToken(TokenType::OP_SHIFT_LEFT_ASS);
                else addToken(TokenType::OP_SHIFT_LEFT);
            } else if (match('=')) {
                addToken(TokenType::OP_LESS_EQ);
            } else {
                addToken(TokenType::OP_LESS);
            }
            break;

        case '>':
            if (match('>')) {
                if (match('=')) addToken(TokenType::OP_SHIFT_RIGHT_ASS);
                else addToken(TokenType::OP_SHIFT_RIGHT);
            } else if (match('=')) {
                addToken(TokenType::OP_GRT_EQ);
            } else {
                addToken(TokenType::OP_GRT);
            }
            break;

        case '&':
            if (match('&')) addToken(TokenType::OP_AND);
            else if (match('=')) addToken(TokenType::OP_AND_ASS);
            else addToken(TokenType::OP_BIT_AND);
            break;

        case '|':
            if (match('|')) addToken(TokenType::OP_OR);
            else if (match('=')) addToken(TokenType::OP_OR_ASS);
            else addToken(TokenType::OP_BIT_OR);
            break;

        // Dot operator vs Range operator vs Float starting with dot
        case '.':
            if (match('.')) {
                addToken(TokenType::RANGE_OP);
            } else if (isdigit(peek())) {
                // E.g. .5 -> back up and parse as a full number
                current--;
                column--;
                number();
            } else {
                addToken(TokenType::DOT);
            }
            break;

        // Division vs Comments
        case '/':
            if (match('/')) {
                // Single line comment: skip until newline
                while (peek() != '\n' && !isAtEnd()) advance();
            } else if (match('*')) {
                // Nested multiline comments
                int nestingLevel = 1;

                while (nestingLevel > 0 && !isAtEnd()) {
                    // Entering a nested comment
                    if (peek() == '/' && peekNext() == '*') {
                        advance(); advance();
                        nestingLevel++;
                        continue;
                    }

                    // Exiting a comment level
                    if (peek() == '*' && peekNext() == '/') {
                        advance(); advance();
                        nestingLevel--;
                        continue;
                    }

                    // Handle internal newlines to keep line count accurate
                    if (peek() == '\n') {
                        line++;
                        column = 0; // Next advance will increment it to 1
                    }
                    advance();
                }

                if (nestingLevel > 0) {
                    // Update start to current so the error token grabs the unclosed part
                    start = current;
                    manageError("Unterminated multiline comment");
                }
            }
            else if (match('=')) {
                addToken(TokenType::OP_DIV_ASS);
            }
            else {
                addToken(TokenType::OP_SLASH);
            }
            break;

        // Whitespace (Inline spaces are safely ignored here)
        case ' ':
        case '\r':
        case '\t':
            break;

        // Structural Whitespace
        case '\n':
            line++;
            column = 1;
            addToken(TokenType::NEWLINE);
            handleIndentation();
            break;

        // Literals
        case '"': string(); break;
        case '\'': charLiteral(); break;

        // Fallback for numbers, identifiers, or invalid characters
        default:
            if (isdigit(c)) {
                current--; column--; // Back up to pass full string to number()
                number();
            } else if (isalpha(c) || c == '_' || (unsigned char)c >= 128) {
                current--; column--;
                identifier();
            } else {
                manageError("Unexpected character");
                recoverFromError();
            }
            break;
    }
}

// --- Token Handlers ---

void Lexer::handleIndentation() {
    int spaces = 0;

    // Use a temporary cursor to lookahead without committing,
    // in case it's an empty line.
    size_t tempCurrent = current;

    // Count physical spaces (tabs count as 4 spaces)
    while (tempCurrent < source.length()) {
        char c = source[tempCurrent];
        if (c == ' ') {
            spaces++;
        } else if (c == '\t') {
            spaces += 4;
        } else {
            break;
        }
        tempCurrent++;
    }

    // Ignore indentation entirely on blank lines or comment-only lines
    if (tempCurrent >= source.length() || source[tempCurrent] == '\n' || source[tempCurrent] == '\r') {
        return; // Empty line, ignore
    }
    if (source[tempCurrent] == '/' && tempCurrent + 1 < source.length() &&
       (source[tempCurrent+1] == '/' || source[tempCurrent+1] == '*')) {
        return; // Comment line, ignore
       }

    // Commit the cursor movement
    column += spaces;
    current = tempCurrent;

    int currentLevel = indentStack.back();

    // Indentation logic to manage block structure
    if (spaces > currentLevel) {
        indentStack.push_back(spaces);
        tokens.emplace_back(TokenType::INDENT, "INDENT", line, column);
    }
    else if (spaces < currentLevel) {
        while (spaces < indentStack.back()) {
            indentStack.pop_back();
            tokens.emplace_back(TokenType::DEDENT, "DEDENT", line, column);
        }

        // If we dedented to a level that doesn't exist in our stack, it's a syntax error
        if (indentStack.back() != spaces) {
            start = current; // Anchor error
            manageError("Inconsistent indentation level");
        }
    }
}

void Lexer::number() {
    TokenType type = TokenType::INT_LITERAL;
    int base = 10;
    size_t offset = 0;

    // Recognize Base Prefixes (0x, 0b, 0o)
    if (peek() == '0') {
        char next = tolower(peekNext());
        if (next == 'x') {
            type = TokenType::HEX_LITERAL;
            base = 16;
            offset = 2;
            advance(); advance();
            while (isxdigit(peek())) advance();
        } else if (next == 'b') {
            type = TokenType::BIN_LITERAL;
            base = 2;
            offset = 2;
            advance(); advance();
            while (peek() == '0' || peek() == '1') advance();
        } else if (next == 'o') {
            type = TokenType::OCT_LITERAL;
            base = 8;
            offset = 2;
            advance(); advance();
            while (peek() >= '0' && peek() <= '7') advance();
        }
    }

    // Parse Decimal or Float
    if (base == 10) {
        while (isdigit(peek())) advance();

        // Check for fractional part
        if (peek() == '.') {
            if (peekNext() == '.') {
                // Range operator lookahead (e.g. 10..20). We stop here.
            } else if (isalpha(peekNext()) || peekNext() == '_') {
                // Method call lookahead (e.g. 10.toString()). We stop here.
            } else {
                // Standard float (e.g. 10.5 10. )
                type = TokenType::FLOAT_LITERAL;
                advance();
                while (isdigit(peek())) advance();
            }
        }

        // Check for scientific notation
        if (tolower(peek()) == 'e') {
            type = TokenType::FLOAT_LITERAL;
            advance();
            if (peek() == '+' || peek() == '-') advance();
            while (isdigit(peek())) advance();
        }
    }

    // Extract and safely convert string to numeric value
    std::string text = source.substr(start, current - start);

     try {
        if (type == TokenType::FLOAT_LITERAL) {
            double value = std::stod(text);
            addToken(type, value); // Chiamato SOLO se non lancia eccezione
        } else {
            std::string cleanText = (offset > 0) ? text.substr(offset) : text;
            long long value = std::stoll(cleanText, nullptr, base);
            addToken(type, value); // Chiamato SOLO se non lancia eccezione
        }
    } catch (...) {
        // Se arriviamo qui, addToken NON è stato chiamato.
        // Chiamiamo manageError e usciamo.
        manageError("Numeric literal is out of range");
    }
}

void Lexer::identifier() {
    while (isalnum(peek()) || peek() == '_' || (unsigned char)peek() >= 128) {
        advance();
    }

    std::string text = source.substr(start, current - start);
    if (peek() == '"') {
        if (text == "r") {
            advance(); // Consuma il "
            rawString();
            return;
        }
        if (text == "f") {
            advance(); // Consuma il "
            formatString();
            return;
        }
    }
    TokenType type;
    int id = 0;

    // Static Hashing for O(1) keyword resolution
    switch (hash(text.c_str())) {
        case hash("int"): type = TokenType::KW_INT; break;
        case hash("float"): type = TokenType::KW_FLOAT; break;
        case hash("char"): type = TokenType::KW_CHAR; break;
        case hash("string"): type = TokenType::KW_STRING; break;
        case hash("void"): type = TokenType::KW_VOID; break;
        case hash("bool"): type = TokenType::KW_BOOL; break;

        case hash("if"): type = TokenType::KW_IF; break;
        case hash("else"): type = TokenType::KW_ELSE; break;
        case hash("elif"): type = TokenType::KW_ELIF; break;
        case hash("while"): type = TokenType::KW_WHILE; break;
        case hash("for"): type = TokenType::KW_FOR; break;
        case hash("in"): type = TokenType::KW_IN; break;
        case hash("return"): type = TokenType::KW_RETURN; break;
        case hash("break"): type = TokenType::KW_BREAK; break;
        case hash("continue"): type = TokenType::KW_CONTINUE; break;

        case hash("struct"): type = TokenType::KW_STRUCT; break;
        case hash("class"): type = TokenType::KW_CLASS; break;

        case hash("true"): type = TokenType::KW_TRUE; break;
        case hash("false"): type = TokenType::KW_FALSE; break;
        case hash("null"): type = TokenType::KW_NULL; break;

        default: type = TokenType::IDENTIFIER; break;
    }

    // String Interning: Assign unique numeric ID to user-defined variables
    if (type == TokenType::IDENTIFIER) {
        if (identifierIDs.find(text) == identifierIDs.end()) {
            identifierIDs[text] = nextID++;
        }
        id = identifierIDs[text];
    }

    addIdentifierToken(type, id);
}

void Lexer::string() {
    std::string processedValue = "";

    while (peek() != '"' && !isAtEnd()) {

        if (peek() == '\n') {
            // Se andiamo a capo, la stringa è considerata non chiusa correttamente
            manageError("Unterminated string literal (newline found)");
            return; // Esci subito, non mangiare le righe successive!
        }

        char c = peek();

        if (c == '\n') line++;

        if (c == '\\') {
            advance(); // Skip the backslash
            if (isAtEnd()) break;

            char escapeChar = peek();
            switch (escapeChar) {
                case 'n': processedValue += '\n'; break;
                case 'r': processedValue += '\r'; break;
                case 't': processedValue += '\t'; break;
                case 'b': processedValue += '\b'; break;
                case 'f': processedValue += '\f'; break;
                case 'v': processedValue += '\v'; break;
                case '\\': processedValue += '\\'; break;
                case '"': processedValue += '"'; break;
                case '\'': processedValue += '\''; break;
                case '0': processedValue += '\0'; break;
                default:
                    // Fallback for strings: keep unknown escape chars as-is
                    processedValue += escapeChar;
                    break;
            }
        } else {
            processedValue += c;
        }
        advance();
    }

    if (isAtEnd()) {
        manageError("Unterminated string literal");
        return;
    }

    advance(); // Skip closing quote
    addToken(TokenType::STRING_LITERAL, processedValue);
}
void Lexer::rawString() {
    std::string value = "";

    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            manageError("Unterminated raw string literal (newline found)");
            return;
        }

        // Nelle raw string, ignoriamo gli escape standard (\n non diventa a capo).
        // Tuttavia, dobbiamo permettere \" altrimenti la stringa si chiuderebbe troppo presto.
        if (peek() == '\\' && peekNext() == '"') {
            advance(); // Consuma \
            value += '\\';
            value += '"';
            advance(); // Consuma "
            continue;
        }

        value += advance(); // Aggiunge il carattere letteralmente
    }

    if (isAtEnd()) {
        manageError("Unterminated raw string literal");
        return;
    }

    advance(); // Consuma la chiusura "
    addToken(TokenType::RAW_STRING_LITERAL, value);
}

void Lexer::formatString() {
    // In questo modo, il Parser saprà che deve cercare le variabili tra { } nel valore.

    std::string processedValue = "";

    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            manageError("Unterminated format string literal (newline found)");
            return;
        }

        char c = peek();
        if (c == '\\') {
            advance(); // Skip \
            if (isAtEnd()) break;
            char escapeChar = peek();
            switch (escapeChar) {
                case 'n': processedValue += '\n'; break;
                case 'r': processedValue += '\r'; break;
                case 't': processedValue += '\t'; break;
                case 'b': processedValue += '\b'; break;
                case 'f': processedValue += '\f'; break;
                case 'v': processedValue += '\v'; break;
                case '\\': processedValue += '\\'; break;
                case '"': processedValue += '"'; break;
                case '\'': processedValue += '\''; break;
                case '0': processedValue += '\0'; break;
                default: processedValue += escapeChar; break;
            }
        } else {
            processedValue += c;
        }
        advance();
    }

    if (isAtEnd()) {
        manageError("Unterminated format string literal");
        return;
    }

    advance(); // Consuma "
    addToken(TokenType::FORMAT_STRING_LITERAL, processedValue);
}
void Lexer::charLiteral() {
    if (isAtEnd() || peek() == '\'') {
        manageError("Empty character literal");
        if (!isAtEnd()) advance();
        return;
    }

    char value = 0;

    if (peek() == '\\') {
        advance();

        if (isAtEnd()) {
            manageError("Unterminated character literal");
            return;
        }

        switch (peek()) {
            case 'n': value = '\n'; break;
            case 'r': value = '\r'; break;
            case 't': value = '\t'; break;
            case 'b': value = '\b'; break;
            case 'f': value = '\f'; break;
            case 'v': value = '\v'; break;
            case '\\': value = '\\'; break;
            case '\'': value = '\''; break;
            case '"': value = '"'; break;
            case '0': value = '\0'; break;
            default:
                advance();
                manageError("Invalid escape sequence in char literal");
                // Error Recovery: skip to the closing quote
                while (!isAtEnd() && peek() != '\'' && peek() != '\n') advance();
                if (peek() == '\'') advance();
                return;
        }
        advance();
    }
    else {
        value = advance();

    }

    // Ensure the literal is properly closed
    if (peek() != '\'') {
        // Capture the multi-character sequence for the error message
        while (!isAtEnd() && peek() != '\'' && peek() != '\n') advance();
        if (peek() == '\'') advance();
        manageError("Unterminated or multi-character char literal");
        return;
    }

    advance(); // Consume the closing quote
    addToken(TokenType::CHAR_LITERAL, value);
}

// Token Constructors

void Lexer::addToken(TokenType type) {
    std::string text = source.substr(start, current - start);
    tokens.emplace_back(type, text, line, startColumn);
}

void Lexer::addIdentifierToken(TokenType type, int id) {
    std::string text = source.substr(start, current - start);
    tokens.emplace_back(type, text, line, startColumn, id);
}

void Lexer::addToken(TokenType type, LiteralValue value) {
    std::string text = source.substr(start, current - start);
    tokens.emplace_back(type, text, line, startColumn, std::move(value));
}
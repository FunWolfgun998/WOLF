#include "../../include/Parser/Parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

// ============================================================================
// STATEMENT PARSERS (Recursive Descent)
// ============================================================================

std::vector<Stmt> Parser::parse() {
    std::vector<Stmt> statements;
    while (!isAtEnd()) {
        try {
            // Ignore extra newlines at the top level
            if (match(TokenType::NEWLINE)) continue;
            statements.push_back(parseStatement());
        } catch (const ParseError& error) {
            synchronize();
        }
    }
    return statements;
}

Stmt Parser::parseStatement() {
    // Variable Declarations
    if (check(TokenType::KW_INT) || check(TokenType::KW_FLOAT) ||
        check(TokenType::KW_CHAR) || check(TokenType::KW_STRING) ||
        check(TokenType::KW_BOOL)) {
        return parseVarDecl();
    }

    // Control Flow
    if (check(TokenType::KW_IF)) return parseIfStmt();
    if (check(TokenType::KW_WHILE)) return parseWhileStmt();

    // Fallback: Expression Statement (e.g., "x = 5" or "functionCall()")
    Expr expr = parseExpression();

    // In WOLF, standalone expressions must end with a newline
    consume(TokenType::NEWLINE, "Expected newline after expression.");

    return makeStmt<ExpressionStmt>(std::move(expr));
}

// --- Specific Statement Builders ---

Stmt Parser::parseVarDecl() {
    // Consume the type token internally
    Token typeToken = advance();

    // Consume the identifier
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected variable name.");

    std::unique_ptr<Expr> initializer = nullptr;

    // Optional initialization (e.g., "= 10")
    if (match(TokenType::OP_ASSIGN)) {
        initializer = std::make_unique<Expr>(parseExpression());
    }

    // Statement terminator
    consume(TokenType::NEWLINE, "Expected newline after variable declaration.");

    return makeStmt<VarDeclStmt>(typeToken, nameToken, std::move(initializer));
}

Stmt Parser::parseIfStmt() {
    // Consume the 'if' token internally
    advance();

    consume(TokenType::L_PAREN, "Expected '(' after 'if'.");
    Expr condition = parseExpression();
    consume(TokenType::R_PAREN, "Expected ')' after if condition.");
    consume(TokenType::COLON, "Expected ':' after if condition.");
    consume(TokenType::NEWLINE, "Expected newline after ':'.");

    BlockStmt thenBranch = parseBlock();

    // Handle multiple 'elif' branches iteratively
    std::vector<ElseIfBranch> elifBranches;
    while (check(TokenType::KW_ELIF)) {
        advance(); // Consume 'elif' internally

        consume(TokenType::L_PAREN, "Expected '(' after 'elif'.");
        Expr cond = parseExpression();
        consume(TokenType::R_PAREN, "Expected ')' after elif condition.");
        consume(TokenType::COLON, "Expected ':' after elif condition.");
        consume(TokenType::NEWLINE, "Expected newline.");

        BlockStmt block = parseBlock();
        elifBranches.push_back({std::move(cond), std::move(block)});
    }

    // Handle optional 'else' branch
    std::unique_ptr<BlockStmt> elseBranch = nullptr;
    if (check(TokenType::KW_ELSE)) {
        advance(); // Consume 'else' internally

        consume(TokenType::COLON, "Expected ':' after 'else'.");
        consume(TokenType::NEWLINE, "Expected newline.");
        elseBranch = std::make_unique<BlockStmt>(parseBlock());
    }

    return makeStmt<IfStmt>(std::move(condition), std::move(thenBranch),
                            std::move(elifBranches), std::move(elseBranch));
}

Stmt Parser::parseWhileStmt() {
    // Consume 'while' internally
    advance();

    consume(TokenType::L_PAREN, "Expected '(' after 'while'.");
    Expr condition = parseExpression();
    consume(TokenType::R_PAREN, "Expected ')' after condition.");
    consume(TokenType::COLON, "Expected ':' after while condition.");
    consume(TokenType::NEWLINE, "Expected newline after ':'.");

    BlockStmt body = parseBlock();

    return makeStmt<WhileStmt>(std::move(condition), std::move(body));
}

BlockStmt Parser::parseBlock() {
    std::vector<Stmt> statements;

    // A block MUST begin with an increase in indentation
    consume(TokenType::INDENT, "Expected indented block.");

    // Parse statements until the block is dedented or EOF is reached
    while (!check(TokenType::DEDENT) && !isAtEnd()) {
        // Safely ignore empty lines inside a block
        if (match(TokenType::NEWLINE)) continue;

        statements.push_back(parseStatement());
    }

    // The block MUST end with a decrease in indentation
    consume(TokenType::DEDENT, "Expected dedent at the end of the block.");

    return BlockStmt{std::move(statements)};
}
// Navigation Helpers
Token Parser::peek() const {
    return tokens[current];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return tokens[current - 1]; // Restituisce quello appena "superato"
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();

    // Trigger Error + Panic Mode
    error(peek(), message);
    throw ParseError();
}

// --- Error Handling ---

void Parser::error(const Token& token, const std::string& message) {
    std::cerr << "[Parse Error] Line " << token.line
              << ", Col " << token.column << ": " << message;

    if (token.type == TokenType::END_OF_FILE) {
        std::cerr << " at end of file.";
    } else {
        std::cerr << " at '" << token.lexeme << "'";
    }
    std::cerr << std::endl;
}

void Parser::synchronize() {
    advance(); // Consume the offending token

    while (!isAtEnd()) {
        // If we see a newline, we are likely at the start of a new statement
        if (peek().type == TokenType::NEWLINE) return;

        // If we see a structure keyword, restart parsing from there
        switch (peek().type) {
            case TokenType::KW_CLASS:
            case TokenType::KW_STRUCT:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_FOR:
            case TokenType::KW_RETURN:
            case TokenType::INDENT:
            case TokenType::DEDENT:
                return;
            default:
                break;
        }
        advance();
    }
}

Expr Parser::parseExpression() {
    // Start parsing with the lowest possible precedence.
    return parsePrecedence(Precedence::ASSIGNMENT);
}

/*
    The core of the Pratt Parser.
    It parses a left side, then enters a loop to "eat" right-side tokens
    as long as their binding power (precedence) is strictly greater than the requested one.
*/
Expr Parser::parsePrecedence(Precedence precedence) {
    // 1. Consume the first token and determine its prefix rule
    Token firstToken = advance();

    NudHandlerMethod prefixRule = getNudHandler(firstToken.type);
    if (prefixRule == nullptr) {
        error(firstToken, "Expected expression.");
        throw ParseError();
    }

    // Build the left side of the tree by passing the consumed token to the rule
    Expr left = (this->*prefixRule)(firstToken);

    // 2. The Precedence Battle: keep consuming if the NEXT operator binds tighter
    // than the current context. We check peek() before consuming.
    while (precedence <= getPrecedence(peek().type)) {

        // The operator is strong enough! Consume it.
        Token opToken = advance();

        LedHandlerMethod infixRule = getLedHandler(opToken.type);
        if (infixRule == nullptr) {
            error(opToken, "Unexpected operator.");
            throw ParseError();
        }

        // Build the new merged node, passing the left side and the operator token
        left = (this->*infixRule)(std::move(left), opToken);
    }

    return left;
}

// ============================================================================
// EXPRESSION PARSERS (Node Builders)
// ============================================================================

Expr Parser::parseLiteral(Token token) {
    // Token holds INT, FLOAT, STRING, CHAR, TRUE, FALSE or NULL
    return makeExpr<LiteralExpr>(token);
}

Expr Parser::parseVariable(Token token) {
    // Token holds the IDENTIFIER
    return makeExpr<VariableExpr>(token);
}

Expr Parser::parseGrouping(Token token) {
    // token is the '(' we just consumed. Now parse the expression inside.
    Expr expr = parseExpression();

    // We expect a closing parenthesis
    consume(TokenType::R_PAREN, "Expected ')' after expression.");

    return makeExpr<GroupingExpr>(std::move(expr));
}

Expr Parser::parseUnary(Token token) {
    // token is the operator, e.g. '-', '!'

    // Parse the right operand with UNARY precedence to bind tightly
    Expr right = parsePrecedence(Precedence::UNARY);

    return makeExpr<UnaryExpr>(token, std::move(right));
}

Expr Parser::parseBinary(Expr left, Token opToken) {
    // opToken is the operator we just consumed, e.g. '+', '*'

    // Find the binding power of this specific operator
    Precedence rulePrecedence = getPrecedence(opToken.type);

    // Parse the right operand with a SLIGHTLY HIGHER precedence.
    // Example: if op is '+', we parse the right side rejecting other '+'
    // but accepting '*'. This guarantees Left-Associativity (1+2+3 -> (1+2)+3).
    Precedence nextPrecedence = static_cast<Precedence>(static_cast<int>(rulePrecedence) + 1);
    Expr right = parsePrecedence(nextPrecedence);

    return makeExpr<BinaryExpr>(std::move(left), opToken, std::move(right));
}

Precedence Parser::getPrecedence(TokenType type) const {
    switch (type) {
        case TokenType::OP_ASSIGN:
        case TokenType::OP_PLUS_ASS: case TokenType::OP_MIN_ASS:
        case TokenType::OP_MUL_ASS: case TokenType::OP_DIV_ASS:
        case TokenType::OP_MOD_ASS: case TokenType::OP_AND_ASS:
        case TokenType::OP_OR_ASS: case TokenType::OP_XOR_ASS:
        case TokenType::OP_SHIFT_LEFT_ASS: case TokenType::OP_SHIFT_RIGHT_ASS:
            return Precedence::ASSIGNMENT;

        case TokenType::OP_OR:
            return Precedence::OR;
        case TokenType::OP_AND:
            return Precedence::AND;
        case TokenType::OP_EQ_EQ: case TokenType::OP_NOT_EQ:
            return Precedence::EQUALITY;
        case TokenType::OP_LESS: case TokenType::OP_LESS_EQ:
        case TokenType::OP_GRT: case TokenType::OP_GRT_EQ:
            return Precedence::COMPARISON;
        case TokenType::OP_PLUS: case TokenType::OP_MINUS:
        case TokenType::OP_BIT_OR: case TokenType::OP_XOR:
            return Precedence::TERM;
        case TokenType::OP_STAR: case TokenType::OP_SLASH:
        case TokenType::OP_MOD: case TokenType::OP_BIT_AND:
        case TokenType::OP_SHIFT_LEFT: case TokenType::OP_SHIFT_RIGHT:
            return Precedence::FACTOR;
        default:
            return Precedence::NONE;
    }
}

Parser::NudHandlerMethod Parser::getNudHandler(TokenType type) const {
    switch (type) {
        case TokenType::INT_LITERAL:
        case TokenType::FLOAT_LITERAL:
        case TokenType::HEX_LITERAL:
        case TokenType::BIN_LITERAL:
        case TokenType::OCT_LITERAL:
        case TokenType::STRING_LITERAL:
        case TokenType::RAW_STRING_LITERAL:
        case TokenType::FORMAT_STRING_LITERAL:
        case TokenType::CHAR_LITERAL:
        case TokenType::KW_TRUE:
        case TokenType::KW_FALSE:
        case TokenType::KW_NULL:
            return &Parser::parseLiteral;

        case TokenType::IDENTIFIER:
            return &Parser::parseVariable;

        case TokenType::L_PAREN:
            return &Parser::parseGrouping;

        case TokenType::OP_MINUS:
        case TokenType::OP_PLUS:
        case TokenType::OP_NOT:
        case TokenType::OP_TILDE:
        case TokenType::OP_INC:
        case TokenType::OP_DEC:
            return &Parser::parseUnary;

        default:
            return nullptr;
    }
}

Parser::LedHandlerMethod Parser::getLedHandler(TokenType type) const {
    switch (type) {
        // Binary operators
        case TokenType::OP_PLUS: case TokenType::OP_MINUS:
        case TokenType::OP_STAR: case TokenType::OP_SLASH:
        case TokenType::OP_MOD: case TokenType::OP_XOR:
        case TokenType::OP_BIT_AND: case TokenType::OP_BIT_OR:
        case TokenType::OP_SHIFT_LEFT: case TokenType::OP_SHIFT_RIGHT:
        case TokenType::OP_EQ_EQ: case TokenType::OP_NOT_EQ:
        case TokenType::OP_LESS: case TokenType::OP_LESS_EQ:
        case TokenType::OP_GRT: case TokenType::OP_GRT_EQ:
        case TokenType::OP_AND: case TokenType::OP_OR:
        case TokenType::OP_ASSIGN:
        case TokenType::OP_PLUS_ASS: case TokenType::OP_MIN_ASS:
        case TokenType::OP_MUL_ASS: case TokenType::OP_DIV_ASS:
        case TokenType::OP_MOD_ASS: case TokenType::OP_AND_ASS:
        case TokenType::OP_OR_ASS: case TokenType::OP_XOR_ASS:
        case TokenType::OP_SHIFT_LEFT_ASS: case TokenType::OP_SHIFT_RIGHT_ASS:
            return &Parser::parseBinary;

        default:
            return nullptr;
    }
}
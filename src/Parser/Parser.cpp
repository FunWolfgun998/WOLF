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
    // 1. Types (Variables OR Functions)
    if (check(TokenType::KW_INT) || check(TokenType::KW_FLOAT) ||
        check(TokenType::KW_CHAR) || check(TokenType::KW_STRING) ||
        check(TokenType::KW_BOOL) || check(TokenType::KW_VOID)) { // Added VOID for functions!

        // Peek at the next token to decide!
        // We know that peekNext(1) is the identifier.  If the peeknNext(2) is a '(' is a function otherwise is a variable declaration.
        if (peeknNext(2).type == TokenType::L_PAREN) {
            return parseFunctionDecl(); // It's a function!
        } else {
            return parseVarDecl(); // It's a variable!
        }
        }

    // 2. Control Flow
    if (match(TokenType::KW_IF)) return parseIfStmt();
    if (match(TokenType::KW_WHILE)) return parseWhileStmt();
    if (match(TokenType::KW_FOR)) return parseForStmt();
    if (match(TokenType::KW_STRUCT)) return parseStructDecl();
    if (match(TokenType::KW_RETURN)) return parseReturnStmt();
    // Fallback: Expression Statement (e.g., "x = 5" or "functionCa ll()")
    Expr expr = parseExpression();

    // In WOLF, standalone expressions must end with a newline
    consume(TokenType::NEWLINE, "Expected newline after expression.");

    return makeStmt<ExpressionStmt>(std::move(expr));
}

// --- Specific Statement Builders ---

Stmt Parser::parseVarDecl() {
    // Consume the type token internally
    Token typeVar = advance();

    // Consume the identifier
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected variable name.");

    std::unique_ptr<Expr> initializer = nullptr;

    // Optional initialization (e.g., "= 10")
    if (match(TokenType::OP_ASSIGN)) {
        initializer = std::make_unique<Expr>(parseExpression());
    }

    // Statement terminator
    consume(TokenType::NEWLINE, "Expected newline after variable declaration.");

    return makeStmt<VarDeclStmt>(typeVar, nameToken, std::move(initializer));
}

Stmt Parser::parseIfStmt() {
    // We already consumed 'if'

    Expr condition = parseExpression();
    consume(TokenType::COLON, "Expected ':' after if condition.");
    consume(TokenType::NEWLINE, "Expected newline after ':'.");

    BlockStmt thenBranch = parseBlock();

    // Handle multiple 'elif' branches iteratively
    std::vector<ElseIfBranch> elifBranches;
    while (match(TokenType::KW_ELIF)) {
        Expr cond = parseExpression();
        consume(TokenType::COLON, "Expected ':' after elif condition.");
        consume(TokenType::NEWLINE, "Expected newline after ':'.");

        BlockStmt block = parseBlock();
        elifBranches.push_back({std::move(cond), std::move(block)});
    }

    // Handle 'else' branch
    std::unique_ptr<BlockStmt> elseBranch = nullptr;
    if (match(TokenType::KW_ELSE)) {
        consume(TokenType::COLON, "Expected ':' after 'else'.");
        consume(TokenType::NEWLINE, "Expected newline after ':'.");
        elseBranch = std::make_unique<BlockStmt>(parseBlock());
    }

    return makeStmt<IfStmt>(std::move(condition), std::move(thenBranch),
                            std::move(elifBranches), std::move(elseBranch));
}

Stmt Parser::parseWhileStmt() {
    // We already consumed 'while'

    Expr condition = parseExpression();
    consume(TokenType::COLON, "Expected ':' after while condition.");
    consume(TokenType::NEWLINE, "Expected newline after ':'.");

    BlockStmt body = parseBlock();

    return makeStmt<WhileStmt>(std::move(condition), std::move(body));
}

Stmt Parser::parseForStmt() {
    // We already consumed 'for'

    Token iteratorVar = consume(TokenType::IDENTIFIER, "Expected variable name after 'for'.");
    consume(TokenType::KW_IN, "Expected 'in' after iterator variable.");

    Expr startRange = parseExpression();
    consume(TokenType::RANGE_OP, "Expected '..' in range expression.");
    Expr endRange = parseExpression();

    consume(TokenType::COLON, "Expected ':' after for range.");
    consume(TokenType::NEWLINE, "Expected newline after ':'.");

    BlockStmt body = parseBlock();

    return makeStmt<ForStmt>(iteratorVar, std::move(startRange), std::move(endRange), std::move(body));
}
Stmt Parser::parseStructDecl() {
    //We already consumed 'struct'

    // Consume the struct name
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected struct name.");

    // Consume the colon and newline
    consume(TokenType::COLON, "Expected ':' after struct name.");
    consume(TokenType::NEWLINE, "Expected newline after ':'.");

    // A struct only contains VarDecls, not any Stmt.
    // So we parse the block manually, enforcing the rules.
    std::vector<VarDeclStmt> fields;

    consume(TokenType::INDENT, "Expected indented block for struct fields.");

    while (!check(TokenType::DEDENT) && !isAtEnd()) {
        if (match(TokenType::NEWLINE)) continue;

        // Fields must start with a type keyword
        if (check(TokenType::KW_INT) || check(TokenType::KW_FLOAT) ||
            check(TokenType::KW_CHAR) || check(TokenType::KW_STRING) ||
            check(TokenType::KW_BOOL)) {

            // Re-use VarDecl logic, but extract the raw struct since
            // parseVarDecl returns a generic Stmt wrapper.
            Stmt declStmt = parseVarDecl();

            // Safely extract the VarDeclStmt from the variant
            auto* varDeclPtr = std::get_if<std::unique_ptr<VarDeclStmt>>(&declStmt.as);
            if (varDeclPtr) {
                // Move the unique_ptr content into our vector.
                // Note: we move the value pointed to (*get()), not the pointer itself,
                // because our AST stores them as values in the vector.
                fields.push_back(std::move(**varDeclPtr));
            } else {
                error(peek(), "Critical parser error: expected VarDeclStmt.");
                throw ParseError();
            }
        } else {
            error(peek(), "Structs can only contain variable declarations.");
            throw ParseError();
        }
    }

    consume(TokenType::DEDENT, "Expected dedent at the end of struct block.");

    return makeStmt<StructDeclStmt>(nameToken, std::move(fields));
}
Stmt Parser::parseReturnStmt() {
    std::unique_ptr<Expr> value = nullptr;

    //If there is an expression to be returned we parse the expression and we save it as value
    if (!check(TokenType::NEWLINE)) {
        value = std::make_unique<Expr>(parseExpression());
    }

    // Expect newline after expression
    consume(TokenType::NEWLINE, "Expected newline after return statement.");

    return makeStmt<ReturnStmt>(std::move(value));
}
Stmt Parser::parseFunctionDecl() {
    Token returnType = advance();
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected function name.");

    consume(TokenType::L_PAREN, "Expected '(' after function name.");

    std::vector<Parameter> parameters;
    if (!check(TokenType::R_PAREN)) { // Se ci sono parametri
        do {
            Token paramType = advance(); // Dovremmo controllare che sia un tipo valido
            Token paramName = consume(TokenType::IDENTIFIER, "Expected parameter name.");
            parameters.push_back(Parameter{paramType, paramName});
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::R_PAREN, "Expected ')' after parameters.");
    consume(TokenType::COLON, "Expected ':' before function body.");
    consume(TokenType::NEWLINE, "Expected newline.");

    BlockStmt body = parseBlock();

    return makeStmt<FunctionDeclStmt>(returnType, nameToken, std::move(parameters), std::move(body));
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
    if (current >= tokens.size()) {
        return tokens.back();
    }
    return tokens[current];
}
Token Parser::peeknNext(int n = 1) const {
    if (current <= -n && n<=0) {
        //if we want to see in the back using negative numbers we check that we aren't too back to have a negative number
        return tokens.front();
    }
    if (current + n >= tokens.size()) {
        return tokens.back();
    }
    return tokens[n + current];
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
Expr Parser::parseCall(Expr left, Token parenToken) {
    std::vector<Expr> arguments;
    if (!check(TokenType::R_PAREN)) {
        do {
            arguments.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::R_PAREN, "Expected ')' after arguments.");
    return makeExpr<CallExpr>(std::move(left), parenToken, std::move(arguments));
}

Expr Parser::parseArrayAccess(Expr left, Token bracketToken) {
    Expr index = parseExpression();
    consume(TokenType::R_BRACKET, "Expected ']' after array index.");
    return makeExpr<ArrayAccessExpr>(std::move(left), std::move(index));
}

Expr Parser::parseMemberAccess(Expr left, Token dotToken) {
    Token name = consume(TokenType::IDENTIFIER, "Expected property name after '.'.");
    return makeExpr<MemberAccessExpr>(std::move(left), name);
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
        case TokenType::OP_OR: return Precedence::OR;
        case TokenType::OP_AND: return Precedence::AND;
        case TokenType::OP_EQ_EQ: case TokenType::OP_NOT_EQ: return Precedence::EQUALITY;
        case TokenType::OP_LESS: case TokenType::OP_LESS_EQ:
        case TokenType::OP_GRT: case TokenType::OP_GRT_EQ: return Precedence::COMPARISON;
        case TokenType::OP_PLUS: case TokenType::OP_MINUS:
        case TokenType::OP_BIT_OR: case TokenType::OP_XOR: return Precedence::TERM;
        case TokenType::OP_STAR: case TokenType::OP_SLASH:
        case TokenType::OP_MOD: case TokenType::OP_BIT_AND:
        case TokenType::OP_SHIFT_LEFT: case TokenType::OP_SHIFT_RIGHT: return Precedence::FACTOR;
        case TokenType::DOT: case TokenType::L_PAREN: case TokenType::L_BRACKET: return Precedence::CALL;
        default: return Precedence::NONE;
    }
}

Parser::NudHandlerMethod Parser::getNudHandler(TokenType type) const {
    switch (type) {
        case TokenType::INT_LITERAL:
        case TokenType::FLOAT_LITERAL:
        case TokenType::STRING_LITERAL:
        case TokenType::KW_TRUE:
        case TokenType::KW_FALSE:
        case TokenType::KW_NULL: return &Parser::parseLiteral;
        case TokenType::IDENTIFIER: return &Parser::parseVariable;
        case TokenType::L_PAREN: return &Parser::parseGrouping;
        case TokenType::OP_MINUS: case TokenType::OP_PLUS:
        case TokenType::OP_NOT: case TokenType::OP_TILDE:
        case TokenType::OP_INC: case TokenType::OP_DEC: return &Parser::parseUnary;
        default: return nullptr;
    }
}

Parser::LedHandlerMethod Parser::getLedHandler(TokenType type) const {
    switch (type) {
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
        case TokenType::L_PAREN: return &Parser::parseCall;

        case TokenType::L_BRACKET: return &Parser::parseArrayAccess;
        case TokenType::DOT: return &Parser::parseMemberAccess;
        default: return nullptr;
    }
}
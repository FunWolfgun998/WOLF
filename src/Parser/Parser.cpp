#include "../../include/Parser/Parser.h"
#include <iostream>
Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

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

// STATEMENT PARSERS (Recursive Descent)
Stmt Parser::parseStatement() {
    // 1. Types (Variables OR Functions)
    if (isTypeToken(peek().type)&&(peeknNext().type == TokenType::IDENTIFIER) ) {
        // We know that peekNext(1) is the identifier.  If the peeknNext(2) is a '(' is a function otherwise is a variable declaration.
        if (peeknNext(2).type == TokenType::L_PAREN) {
            return parseFunctionDecl(); // It's a function!
        }
        return parseVarDecl(); // It's a variable!
    }

    // 2. Control Flow
    if (check(TokenType::KW_IF)) return parseIfStmt();
    if (check(TokenType::KW_WHILE)) return parseWhileStmt();
    if (check(TokenType::KW_FOR)) return parseForStmt();
    if (check(TokenType::KW_STRUCT)) return parseStructDecl();
    if (check(TokenType::KW_RETURN)) return parseReturnStmt();
    if (check(TokenType::KW_BREAK)) return parseBreakStmt();
    if (check(TokenType::KW_CONTINUE)) return parseContinueStmt();
    // Fallback: Expression Statement (e.g., "x = 5" or "functionCall()")
    Expr expr = parseExpression();

    // In WOLF, standalone expressions must end with a newline
    consumeStatementEnd();

    return makeStmt<ExpressionStmt>(std::move(expr));
}
BlockStmt Parser::parseBlock() {
    std::vector<Stmt> statements;

    // A block MUST begin with an increase in indentation
    consume(TokenType::INDENT, "Expected indented block.");

    // Parse statements until the block finds dedent or EOF is reached
    while (!check(TokenType::DEDENT) && !isAtEnd()) {
        // Safely ignore empty lines inside a block
        if (match(TokenType::NEWLINE)) continue;

        statements.push_back(parseStatement());
    }

    // The block MUST end with a decrease in indentation
    consume(TokenType::DEDENT, "Expected dedent at the end of the block.");

    return BlockStmt{std::move(statements)};
}
Stmt Parser::parseVarDecl() {

    // Consume the type token internally
    if (!isTypeToken(peek().type)) {
        error(peek(), "Expected type name for Variable declaration parsing.");
        throw ParseError();
    }

    Token typeVar = advance();

    // Consume the identifier
    Token nameVariable = consume(TokenType::IDENTIFIER, "Expected variable name.");

    std::unique_ptr<Expr> initializer = nullptr;

    // Optional initialization (e.g., "= 10")
    if (match(TokenType::OP_ASSIGN)) {
        initializer = std::make_unique<Expr>(parseExpression());
    }

    // Statement terminator
    consumeStatementEnd();

    return makeStmt<VarDeclStmt>(typeVar, nameVariable, std::move(initializer));
}
Stmt Parser::parseFunctionDecl() {
    Token returnType = advance();
    Token nameFunction = consume(TokenType::IDENTIFIER, "Expected function name.");

    consume(TokenType::L_PAREN, "Expected '(' after function name.");

    std::vector<Parameter> parameters;
    // If there are parameters
    if (!check(TokenType::R_PAREN)) {
        do {
            Token paramType = advance(); // We should check if the type is valid but that happened on the semantic
            Token paramName = consume(TokenType::IDENTIFIER, "Expected parameter name.");
            std::unique_ptr<Expr> defValue = nullptr;
            if (match(TokenType::OP_ASSIGN)) {
                defValue = std::make_unique<Expr>(parseExpression());
            }
            parameters.push_back(Parameter{paramType, paramName, std::move(defValue)});
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::R_PAREN, "Expected ')' after parameters.");
    consume(TokenType::COLON, "Expected ':' before function body.");
    consume(TokenType::NEWLINE, "Expected newline before function body.");

    Stmt body = makeStmt<BlockStmt>(parseBlock());
    return makeStmt<FunctionDeclStmt>(returnType, nameFunction, std::move(parameters), std::move(body));
}
Stmt Parser::parseStructDecl() {

    Token keyword = consume(TokenType::KW_STRUCT, "Expected 'struct' for struct parsing.");
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected struct name.");

    consume(TokenType::COLON, "Expected ':' after struct name.");
    consume(TokenType::NEWLINE, "Expected newline after ':'.");

    std::vector<Stmt> fields;

    consume(TokenType::INDENT, "Expected indented block for struct fields.");

    while (!check(TokenType::DEDENT) && !isAtEnd()) {
        if (match(TokenType::NEWLINE)) continue;

        // 2. THE FIX: A field can start with a Type Keyword OR an Identifier (User-defined type)
        if (isTypeToken(peek().type)) {

            // Parse the declaration once
            Stmt declStmt = parseVarDecl();

            // 3. Move the parsed statement into the vector
            // We use std::move because declStmt contains a unique_ptr
            fields.push_back(std::move(declStmt));

        } else {
            error(peek(), "Structs can only contain variable declarations.");
            throw ParseError();
        }
    }

    consume(TokenType::DEDENT, "Expected dedent at the end of struct block.");

    // Return the completed struct declaration
    return makeStmt<StructDeclStmt>(nameToken, std::move(fields));
}
Stmt Parser::parseIfStmt() {
    Token keyword= consume(TokenType::KW_IF, "Expected 'if' for if parsing");

    Expr condition = parseExpression();
    consume(TokenType::COLON, "Expected ':' after if condition. ");
    consume(TokenType::NEWLINE, "Expected newline after ':'. ");

    // Wrap the BlockStmt into a Stmt
    Stmt thenBranch = makeStmt<BlockStmt>(parseBlock());

    // Handle multiple 'elif' branches iteratively
    std::vector<ElseIfBranch> elifBranches;
    while (check(TokenType::KW_ELIF)) {
        Token elifKeyword = advance();
        Expr cond = parseExpression();
        consume(TokenType::COLON, "Expected ':' after elif condition.");
        consume(TokenType::NEWLINE, "Expected newline after ':'.");

        // Wrap the Elif block
        elifBranches.push_back({elifKeyword, std::move(cond), makeStmt<BlockStmt>(parseBlock())});
    }

    // Handle 'else' branch
    std::unique_ptr<Stmt> elseBranch = nullptr;
    if (match(TokenType::KW_ELSE)) {
        consume(TokenType::COLON, "Expected ':' after 'else'.");
        consume(TokenType::NEWLINE, "Expected newline after ':'.");
        // Wrap and put into unique_ptr
        elseBranch = std::make_unique<Stmt>(makeStmt<BlockStmt>(parseBlock()));
    }

    return makeStmt<IfStmt>(keyword,std::move(condition), std::move(thenBranch),
                            std::move(elifBranches), std::move(elseBranch));
}
Stmt Parser::parseWhileStmt() {
    Token keyword=  consume(TokenType::KW_WHILE, "Expected 'while' for while parsing");
    Expr condition = parseExpression();
    consume(TokenType::COLON, "Expected ':' after while condition.");
    consume(TokenType::NEWLINE, "Expected newline after ':'.");

    // Wrap the body
    Stmt body = makeStmt<BlockStmt>(parseBlock());

    return makeStmt<WhileStmt>(keyword, std::move(condition), std::move(body));
}
Stmt Parser::parseForStmt() {
    Token keyword=  consume(TokenType::KW_FOR, "Expected 'for' in For parsing");

    Token iteratorVar = consume(TokenType::IDENTIFIER, "Expected variable name after 'for'.");
    consume(TokenType::KW_IN, "Expected 'in' after iterator variable.");

    Expr iterable = parseExpression();

    consume(TokenType::COLON, "Expected ':' after the range.");

    consume(TokenType::NEWLINE, "Expected newline after ':'.");

    Stmt body = makeStmt<BlockStmt>(parseBlock());

    return makeStmt<ForStmt>(keyword, iteratorVar, std::move(iterable), std::move(body));
}
Stmt Parser::parseReturnStmt() {
    // return "Hello XD" + variableWithALongName
    Token keyword=  consume(TokenType::KW_RETURN, "Expected 'return' for return parsing");

    // "Hello XD" + variableWithALongName
    std::unique_ptr<Expr> returnedValue = nullptr;
    if (!(check(TokenType::NEWLINE) || check(TokenType::END_OF_FILE))) {
        returnedValue = std::make_unique<Expr>(parseExpression());
    }
    consumeStatementEnd();

    return makeStmt<ReturnStmt>(keyword, std::move(returnedValue));
}
Stmt Parser::parseBreakStmt() {
    Token keyword=  consume(TokenType::KW_BREAK, "Expected 'break' for break parsing");
    // Expect newline after break
    consumeStatementEnd();

    return makeStmt<BreakStmt>(keyword);
}
Stmt Parser::parseContinueStmt() {
    Token keyword=  consume(TokenType::KW_CONTINUE, "Expected 'continue' for continue parsing");
    // Expect newline after continue
    consumeStatementEnd();

    return makeStmt<ContinueStmt>(keyword);
}

// EXPRESSION PARSERS (Node Builders)
Expr Parser::parseExpression() {
    // Start parsing with the lowest possible precedence.
    return parsePrecedence(Precedence::ASSIGNMENT);
}
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
Expr Parser::parseLiteral(Token token) {
    // Token holds INT, FLOAT, STRING, CHAR, TRUE, FALSE or NULL
    return makeExpr<LiteralExpr>(token);
}
Expr Parser::parseVariable(Token token) {
    // Token holds the IDENTIFIER
    return makeExpr<VariableExpr>(token);
}
// parseGrouping handles ( expr )
Expr Parser::parseGrouping(Token openingParen) {
    Expr expr = parseExpression();
    consume(TokenType::R_PAREN, "Expected ')' after expression.");
    return makeExpr<GroupingExpr>(openingParen,std::move(expr));
}
Expr Parser::parseArrayLiteral(Token openingBracket) {
    std::vector<Expr> elements;
    if (!check(TokenType::R_BRACKET)) {
        do {
            elements.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::R_BRACKET, "Expected ']' after array literal.");
    return makeExpr<ArrayLiteralExpr>(openingBracket, std::move(elements));
}
Expr Parser::parseUnary(Token opToken) {
    // token is the operator, e.g. '-', '!'
    // Parse the right operand with UNARY precedence to bind tightly
    Expr right = parsePrecedence(Precedence::UNARY);
    return makeExpr<UnaryExpr>(opToken, std::move(right));
}

Expr Parser::parsePostfix(Expr left, Token opToken) {
    // Postfix operators only act on the left operand.
    // There is no right operand to parse, so we just return the AST node.
    return makeExpr<UnaryExpr>(opToken, std::move(left), true);
}

Expr Parser::parseBinary(Expr left, Token opToken) {
    // opToken is the operator we just consumed, e.g. '+', '*'
    Precedence rulePrecedence = getPrecedence(opToken.type);
    // Find the binding power of this specific operator
    Precedence nextPrecedence;
    if (rulePrecedence == Precedence::ASSIGNMENT) {
        nextPrecedence = rulePrecedence;
    } else {
        nextPrecedence = static_cast<Precedence>(static_cast<int>(rulePrecedence) + 1);
    }

    // Parse the right operand with a SLIGHTLY HIGHER precedence.
    // Example: if op is '+', we parse the right side rejecting other '+'
    // but accepting '*'. This guarantees Left-Associativity (1+2+3 -> (1+2)+3).

    Expr right = parsePrecedence(nextPrecedence);

    return makeExpr<BinaryExpr>(std::move(left), opToken, std::move(right));

}
Expr Parser::parseRange(Expr left, Token opToken) {
    Expr right = parsePrecedence(Precedence::COMPARISON);
    return makeExpr<RangeExpr>(std::move(left), opToken, std::move(right));
}
Expr Parser::parseTernary(Expr condition, Token questionMark) {

    // Parse the left expression (true branch)
    Expr trueBranch = parseExpression();

    consume(TokenType::COLON, "Expected ':' after true branch on ternary operator.");

    // Parse the right expression (false branch)
    // We use ASSIGNMENT precedence to allow chained ternary operators
    Expr falseBranch = parsePrecedence(Precedence::ASSIGNMENT);

    return makeExpr<TernaryExpr>(std::move(condition), questionMark, std::move(trueBranch), std::move(falseBranch));
}
Expr Parser::parseCall(Expr callee, Token openingParen) {
    std::vector<Expr> arguments;
    if (!check(TokenType::R_PAREN)) {
        do {
            arguments.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::R_PAREN, "Expected ')' after arguments.");
    return makeExpr<CallExpr>(std::move(callee), openingParen, std::move(arguments));
}
Expr Parser::parseArrayAccess(Expr indexed, Token openingBracket) {
    Expr index = parseExpression();
    consume(TokenType::R_BRACKET, "Expected ']' after array index.");
    return makeExpr<ArrayAccessExpr>(std::move(indexed),openingBracket, std::move(index));
}

Expr Parser::parseMemberAccess(Expr accessed, Token dot) {
    Token member = consume(TokenType::IDENTIFIER, "Expected property name after '.'.");
    return makeExpr<MemberAccessExpr>(std::move(accessed), dot, member);
}

// PRATT PARSER HELPERS
/* The core of the Pratt Parser:
    It parses a left side, then enters a loop to "eat" right-side tokens
    as long as their binding power (precedence) is strictly greater than the requested one. */
Precedence Parser::getPrecedence(TokenType type) const {
    switch (type) {
        case TokenType::OP_ASSIGN:
        case TokenType::OP_PLUS_ASS: case TokenType::OP_MIN_ASS:
        case TokenType::OP_MUL_ASS: case TokenType::OP_DIV_ASS:
        case TokenType::OP_MOD_ASS: case TokenType::OP_AND_ASS:
        case TokenType::OP_OR_ASS: case TokenType::OP_XOR_ASS:
        case TokenType::OP_SHIFT_LEFT_ASS: case TokenType::OP_SHIFT_RIGHT_ASS:
        case TokenType::OP_QUESTION: return Precedence::ASSIGNMENT;
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
        case TokenType::RANGE_OP: return Precedence::COMPARISON;
        case TokenType::DOT: case TokenType::L_PAREN: case TokenType::L_BRACKET: return Precedence::CALL;
        case TokenType::OP_INC:
        case TokenType::OP_DEC:
            return Precedence::CALL;
        default: return Precedence::NONE;
    }
}
//Handel Null Denotation -> there is nothing on its left, how this token is going to do/behave?
Parser::NudHandlerMethod Parser::getNudHandler(TokenType type) const {
    switch (type) {
        case TokenType::INT_LITERAL:
        case TokenType::FLOAT_LITERAL:
        case TokenType::HEX_LITERAL:
        case TokenType::BIN_LITERAL:
        case TokenType::OCT_LITERAL:
        case TokenType::CHAR_LITERAL:
        case TokenType::STRING_LITERAL:
        case TokenType::RAW_STRING_LITERAL:
        case TokenType::FORMAT_STRING_LITERAL:
        case TokenType::KW_TRUE:
        case TokenType::KW_FALSE:
        case TokenType::KW_NULL: return &Parser::parseLiteral;
        case TokenType::IDENTIFIER: return &Parser::parseVariable;
        case TokenType::L_PAREN: return &Parser::parseGrouping;
        case TokenType::L_BRACKET: return &Parser::parseArrayLiteral;
        case TokenType::OP_MINUS: case TokenType::OP_PLUS:
        case TokenType::OP_NOT: case TokenType::OP_TILDE:
        case TokenType::OP_INC: case TokenType::OP_DEC: return &Parser::parseUnary;

        default: return nullptr;
    }
}
//Handel Left Denotation -> there is something on its left, how this token is going to do/behave?
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
        case TokenType::OP_INC: case TokenType::OP_DEC: return &Parser::parsePostfix;
        case TokenType::L_PAREN: return &Parser::parseCall;
        case TokenType::L_BRACKET: return &Parser::parseArrayAccess;
        case TokenType::RANGE_OP: return &Parser::parseRange;
        case TokenType::DOT: return &Parser::parseMemberAccess;
        case TokenType::OP_QUESTION: return &Parser::parseTernary;
        default: return nullptr;
    }
}

// NAVIGATION HELPERS
// Advance return the current token and move the cursor on the next token
Token Parser::advance() {
    if (!isAtEnd()) current++;
    return tokens[current - 1]; // Return just skipped one
}
// Peek return the current token
Token Parser::peek() const {
    if (current >= tokens.size()) {
        return tokens.back();
    }
    return tokens[current];
}
// PeeknNext return the token at the position current + n. n can be negative to allow check "behind"
Token Parser::peeknNext(int n) const {
    if (n<=0 && current <= -n ) {
        //if we want to see in the back using negative numbers we check that we aren't too back to have a negative number
        return tokens.front();
    }
    if (current + n >= tokens.size()) {
        return tokens.back();
    }
    return tokens[n + current];
}
// Consume advance the current token if the type is correct otherwise throw error
Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    // Trigger Error + Panic Mode
    error(peek(), message);
    throw ParseError();
}
// Match "eats" the token when making the comparison
bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}
// Check peek at the token type when making the comparison
bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}
bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}
// isTypeToken check if the token is a possible data type (int, float, char, Person <- Struct/Class)
bool Parser::isTypeToken(TokenType type) const {
    switch (type) {
        case TokenType::KW_INT:
        case TokenType::KW_FLOAT:
        case TokenType::KW_CHAR:
        case TokenType::KW_STRING:
        case TokenType::KW_BOOL:
        case TokenType::KW_VOID:
        case TokenType::IDENTIFIER:
            return true;
        default:
            return false;
    }
}

void Parser::consumeStatementEnd() {
    std::string erMessage = "Expected newline";
    if (check(TokenType::NEWLINE)) {
        erMessage+=" at the end of the statement.";
        advance();
    } else if (isAtEnd()) {
    } else {
        error(peek(), erMessage);
        throw ParseError();
    }
}

//ERROR HANDLING
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
// Synchronize advance until a "safe" token is found
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
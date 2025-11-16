#include "parser/Parser.h"

#include <iostream>

#include "ast/nodes/AllNodes.h"
#include <set>


#include "parser/ParseError.h"
#include <stdexcept>

#include "ast/nodes/BlockNode.h"
#include "ast/nodes/IfNode.h"

Parser::Parser(std::vector<Token> tokens)
    : tokens(std::move(tokens)), pos(0) {}

ASTNodePtr Parser::ParseTokens() {
    std::vector<ASTNodePtr> statements;

    while (!Is(TypeToken::End)) {
        consumeNewlines();
        if (auto stmt = parseStatement()) {  // Controlla nullptr
            statements.push_back(std::move(stmt));
        }
    }

    return std::make_unique<ProgramNode>(std::move(statements));
}
void Parser::consumeNewlines() {
    while (Is(TypeToken::Newline)|| Is(TypeToken::Comment)) {
        consumeToken();
    }
}

// Helper functions implementation

const Token& Parser::currentToken() const {
    return tokens[pos];
}

const Token& Parser::consumeToken() {
            std::cout << "Type: " << static_cast<int>(currentToken().type)
                      << ", Value: '" << currentToken().value << "'" << std::endl;
    return tokens[pos++];
}

bool Parser::Is(TypeToken type) const {
    return tokens[pos].type == type;
}

bool Parser::IsNext(TypeToken type) const {
    return tokens[pos+1].type == type;
}

void Parser::expect(TypeToken type, const std::string& errorMsg) {
    if (!Is(type)) {
        throw std::runtime_error(errorMsg + ". Found: " + std::to_string(static_cast<int>(currentToken().type)));
    }
    consumeToken();
}

void Parser::peek(TypeToken type, const std::string& errorMsg) {
    if (!Is(type)) {
        throw std::runtime_error(errorMsg + ". Found: " + std::to_string(static_cast<int>(currentToken().type)));
    }
}

ASTNodePtr Parser::parseStatement() {
    consumeNewlines();
    if (isTypeToken(currentToken().type)) {//int,string,float,char
        return parseDeclaration();
    }
    if (Is(TypeToken::Identifier)) {
        if(isAssignmentOperator(tokens[pos+1].type)) {
            return parseAssignment();
        }
    }
    if (Is(TypeToken::If)) {
        return parseIf();
    }
    if (Is(TypeToken::End)) {

    }
    /* need to manage if al lots of indent with no code or if code
    if (currentToken().type == TypeToken::Indent) {

    }
    if (currentToken().type == TypeToken::Dedent) {

    }
    */

    std::cout << currentToken().value<< std::endl;
    std::cout << static_cast<int>(currentToken().type)<< std::endl;
    throw ParseError("Unexpected token at statement start");
}

ASTNodePtr Parser::parseDeclaration() {
    std::string typeStr = consumeToken().value;

    peek(TypeToken::Identifier, "Expected variable name");
    std::string Identifier = consumeToken().value;

    // Simple declaration (int x)
    if (isEndOfStatement()) {
        consumeNewlines();
        return std::make_unique<VariableDeclNode>(typeStr, Identifier);
    }

    // Initialization (int x = 42)
    if (Is(TypeToken::OpAssign)) {
        consumeToken();
        auto initValue = parseExpression();

        if (!isEndOfStatement()) {
            throw ParseError("Expected newline after initialization");
        }
        consumeNewlines();
        return std::make_unique<VariableInitNode>(typeStr, Identifier, std::move(initValue));
    }

    throw std::runtime_error("Invalid variable declaration");
}

ASTNodePtr Parser::parseAssignment() {
    std::string Identifier = consumeToken().value;

    if (!isAssignmentOperator(currentToken().type)) {
        ParseError("Expected assignment sign");
    }
    std::string typeofsign = consumeToken().value;

        auto initValue = parseExpression();

        if (!isEndOfStatement()) {
            throw ParseError("Expected newline after initialization");
        }
        consumeNewlines();

        return std::make_unique<AssignmentNode>(Identifier,typeofsign, std::move(initValue));
}
ASTNodePtr Parser::parseIf() {
    expect(TypeToken::If, "Expected 'if'");
    expect(TypeToken::LeftParen, "Expected '(' after 'if'");
    auto condition = parseExpression();
    expect(TypeToken::RightParen, "Expected ')' after condition");
    expect(TypeToken::Colon, "Expected ':' after condition");
    expect(TypeToken::Newline, "Expected newline after ':'");
    expect(TypeToken::Indent, "Expected indentation");

    auto thenBlock = parseBlock();

    std::vector<std::pair<ASTNodePtr, ASTNodePtr>> elifConditionsAndBlocks;  // Memorizza (condizione, blocco)

    while (Is(TypeToken::Elif)) {
        consumeToken();
        expect(TypeToken::LeftParen, "Expected '(' after 'elif'");
        auto elifCondition = parseExpression();
        expect(TypeToken::RightParen, "Expected ')' after condition");
        expect(TypeToken::Colon, "Expected ':' after condition");
        expect(TypeToken::Newline, "Expected newline after ':'");
        expect(TypeToken::Indent, "Expected indentation");

        auto elifBlock = parseBlock();
        elifConditionsAndBlocks.emplace_back(std::move(elifCondition), std::move(elifBlock));
        consumeNewlines();
    }

    // Costruisci la catena di elif in ordine inverso
    ASTNodePtr elseBlock = nullptr;
    if (Is(TypeToken::Else)) {
        consumeToken();  // Consuma 'else'
        expect(TypeToken::Colon, "Expected ':' after 'else'");
        expect(TypeToken::Newline, "Expected newline after ':'");
        expect(TypeToken::Indent, "Expected indentation");

        auto elseBlockContent = parseBlock();
        elseBlock = std::make_unique<ElseNode>(std::move(elseBlockContent));
    }

    // Aggiungi gli elif in ordine inverso (per mantenere l'ordine corretto)
    for (auto it = elifConditionsAndBlocks.rbegin(); it != elifConditionsAndBlocks.rend(); ++it) {
        elseBlock = std::make_unique<IfNode>(std::move(it->first), std::move(it->second), std::move(elseBlock),true);

    }

    return std::make_unique<IfNode>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
}

ASTNodePtr Parser::parseBlock() {
        std::vector<ASTNodePtr> statements;

    while (currentToken().type != TypeToken::Dedent && !Is(TypeToken::End)) {
        consumeNewlines();
        if (auto stmt = parseStatement()) {  // Controlla nullptr
            statements.push_back(std::move(stmt));
        }
    }

    std::cout << "Done block "<< std::endl;
    consumeToken();
    return std::make_unique<BlockNode>(std::move(statements));
}
ASTNodePtr Parser::parseExpression() {
    auto left = parsePrimary();
    return parseBinaryOp(std::move(left), 0);
}

ASTNodePtr Parser::parsePrimary() {
    if (isTypeValueToken(currentToken().type)) {
        auto token = consumeToken();

        switch(token.type) {
            case TypeToken::IntValue:
                return std::make_unique<IntLiteralNode>(std::stoi(token.value));
            case TypeToken::FloatValue:
                return std::make_unique<FloatLiteralNode>(std::stod(token.value));
            case TypeToken::CharValue:
                return std::make_unique<CharLiteralNode>(token.value.empty() ? '\0' : token.value[0]);
            case TypeToken::StringValue:
                return std::make_unique<StringLiteralNode>(token.value);
            case TypeToken::True:
            case TypeToken::False:
                return std::make_unique<BoolLiteralNode>(token.type == TypeToken::True);
            default:
                throw ParseError("Unsupported literal type");
        }
    }

    if (Is(TypeToken::Identifier)) {
        auto token = consumeToken();
        return std::make_unique<IdentifierNode>(token.value);
    }

    if (Is(TypeToken::LeftParen)) {
        consumeToken(); // Consume '('
        auto expr = parseExpression();
        expect(TypeToken::RightParen, "Expected ')' after expression");
        return expr;
    }

    std::cout << static_cast<int>(tokens[pos-1].type)<< std::endl;
    throw ParseError("Unexpected token in expression: " + static_cast<int>(tokens[pos-1].type));
}

ASTNodePtr Parser::parseBinaryOp(ASTNodePtr left, int minPrecedence) {
    while (true) {
        auto opToken = currentToken();
        if (!isOperatorStart(opToken.type)) break;

        int precedence = getOperatorPrecedence(opToken.type);
        if (precedence < minPrecedence) break;

        consumeToken(); // Consume operator
        auto right = parsePrimary();

        // Handle operator precedence
        while (true) {
            auto nextOp = currentToken();
            if (!isOperatorStart(nextOp.type)) break;

            int nextPrecedence = getOperatorPrecedence(nextOp.type);
            if (nextPrecedence <= precedence) break;

            right = parseBinaryOp(std::move(right), nextPrecedence);
        }

        left = std::make_unique<BinaryExprNode>(std::move(left),opToken.value,  std::move(right));
    }

    return left;
}
// Utility functions implementation
bool Parser::isTypeToken(TypeToken type) const {
    static const std::set<TypeToken> typeTokens = {
        TypeToken::Int, TypeToken::Float,
        TypeToken::Char, TypeToken::String
    };
    return typeTokens.count(type) > 0;
}
// Utility functions implementation
bool Parser::isAssignmentOperator(TypeToken type) const {
    static const std::set<TypeToken> typeTokens = {
        TypeToken::OpAssign, TypeToken::OpDivideAssign,
        TypeToken::OpMinusAssign, TypeToken::OpMultiplyAssign,
        TypeToken::OpPlusAssign,
    };
    return typeTokens.count(type) > 0;
}
// Utility functions implementation
bool Parser::isTypeValueToken(TypeToken type) const {
    static const std::set<TypeToken> typeTokens = {
        TypeToken::IntValue, TypeToken::FloatValue,
        TypeToken::CharValue, TypeToken::StringValue
    };
    return typeTokens.count(type) > 0;
}

int Parser::getOperatorPrecedence(TypeToken op) const {
    static const std::unordered_map<TypeToken, int> precedence = {
        {TypeToken::OpMultiply, 5},
        {TypeToken::OpDivide, 5},
        {TypeToken::OpPlus, 4},
        {TypeToken::OpMinus, 4},
    };
    return precedence.count(op) ? precedence.at(op) : 0;
}

bool Parser::isOperatorStart(TypeToken type) const {
    static const std::set<TypeToken> operators = {
        TypeToken::OpPlus, TypeToken::OpMinus,
        TypeToken::OpMultiply, TypeToken::OpDivide,
        // ... altri operatori
    };
    return operators.count(type) > 0;
}

bool Parser::isEndOfStatement() const {
    return Is(TypeToken::Newline) || Is(TypeToken::End);
}
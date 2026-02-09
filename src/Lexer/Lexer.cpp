#include <iostream>

#include "../../include/Lexer/Lexer.h"
#include "../../include/Lexer/TokenType.h"
#include "../../include/Lexer/Token.h"

Lexer::Lexer(std::string input)
    : input{std::move(input)},
      pos{0},
      indentStack{0},
      Tokens{},
      currentState{Token::Neutral},
      atLineStart{true}
{
}

constexpr uint32_t hash(const char* str, uint32_t h = 0)
{
    return !*str ? h : hash(str + 1, (h << 5) - h + *str);
}

std::vector<Token> Lexer::getAllTokens()
{
    while (pos < input.size())
    {
        CreateToken();
    }
    if (pos >= input.size())
    {
        //End of File
        Tokens.emplace_back(TypeToken::End, "EOF");
    }
    return Tokens;
}

void Lexer::CreateToken()
{
    switch (currentState)
    {
    case Token::Neutral:
        {
            if (currentChar() == '\n')
            {
                Tokens.emplace_back(TypeToken::Newline, "\\n");
                currentState = Token::Indentation;
                break;
            }
            if (currentChar() == ' ')
            {
                consumeChar();
                break;
            }
            if (isOperator())
            {
                currentState = Token::Op;
                break;
            }
            if (std::isalpha(currentChar()) || currentChar() == '_')
            {
                currentState = Token::Identifier;
                break;
            }
            if (std::isdigit(currentChar()) || currentChar() == '.')
            {
                currentState = Token::Number;
                break;
            }
            if (currentChar() == '\'')
            {
                currentState = Token::Char;
                break;
            }
            if (currentChar() == '\"')
            {
                currentState = Token::String;
                break;
            }

        }
    case Token::Indentation:
        {
            int count = 0;
            while (pos < input.size())
            {
                char c = input[pos++];
                if (c == ' ') count++;
                else if (c == '\t') count += tab_value;
                else break;
            }
            int indent = count / tab_value;
            int lastIndent = indentStack.back();
            atLineStart = false;
            if (indent == lastIndent)
            {
                currentState = Token::Neutral;
                break;
            }
            if (indent > lastIndent)
            {
                while (lastIndent < indent)
                {
                    lastIndent++;
                    indentStack.push_back(lastIndent);
                    Tokens.emplace_back(TypeToken::Indent, std::to_string(lastIndent));
                }
                currentState = Token::Neutral;
                break;
            }
            if (indent < lastIndent)
            {
                while (!indentStack.empty() && indentStack.back() > indent)
                {
                    indentStack.pop_back();
                    Tokens.emplace_back(TypeToken::Dedent, std::to_string(indentStack.back()));
                }
                currentState = Token::Neutral;
                break;
            }
        }
    case Token::Op:
        {
            std::string op (1, currentChar());
            consumeChar();
            static const std::string operatorSecondChar = "=&|<>";
            if (pos + 1 < input.size() && operatorSecondChar.find(input[pos+1]) != std::string::npos)
            {
                op+=consumeChar();
            }
            Tokens.emplace_back(getOperatorToken(op), op);
            currentState = Token::Neutral;
            break;
        }
    case Token::Identifier:
        {
            size_t start = pos;
            while (pos < input.size() && (std::isalnum(input[pos]) || input[pos] == '_'))
            {
                pos++;
            }
            std::string word = input.substr(start, pos - start);
            TypeToken type = getKeywordToken(word);
            Tokens.emplace_back(type, word);
            currentState = Token::Neutral;
            break;
        }
    case Token::Number: {
            const char* start = input.data() + pos;
            char* end;
            strtod(start, &end);

            if (end == start) {
                Tokens.emplace_back(TypeToken::Unknown, std::string(1, currentChar()));
                consumeChar();
            } else {
                pos += end - start;
                bool is_float = (memchr(start, '.', end - start) != nullptr) ||
                               (memchr(start, 'e', end - start) != nullptr) ||
                               (memchr(start, 'E', end - start) != nullptr);

                Tokens.emplace_back(is_float ? TypeToken::FloatValue : TypeToken::IntValue,std::string(start, end - start));
            }
            currentState = Token::Neutral;
            break;
    }
    case Token::SigleLineComment:
        {
            size_t start = pos;
            while (pos < input.size() && input[pos] != '\n')
            {
                consumeChar();
            }
            std::string comment = input.substr(start, pos - start);
            Tokens.emplace_back(TypeToken::Comment, comment);

            currentState = Token::Neutral;
            break;
        }
    case Token::MultiLineComment:
        {
            const size_t start = pos;
            while (pos + 1 < input.size())
            {
                if (input[pos] == '*' && input[pos + 1] == '/')
                {
                    Tokens.emplace_back(TypeToken::Comment, input.substr(start, pos - start));
                    currentState = Token::Neutral;
                    pos += 2;
                    return;
                }
                consumeChar();
            }

            Tokens.emplace_back(TypeToken::Comment, input.substr(start, pos - start));
            currentState = Token::Neutral;
            break;
        }
    case Token::Char:
        {
            const size_t start_pos = pos;
            auto fail = [&]()
            {
                Tokens.emplace_back(TypeToken::Unknown, input.substr(start_pos, pos - start_pos));
                currentState = Token::Neutral;
            };

            if (pos >= input.size())
            {
                fail();
                break;
            }

            char char_value = ' ';
            if (currentChar() == '\\')
            {
                // Escape char
                consumeChar();
                if (pos >= input.size())
                {
                    fail();
                    break;
                }

                switch (currentChar())
                {
                case 'n': char_value = '\n';
                    break;
                case 't': char_value = '\t';
                    break;
                case '\'': char_value = '\'';
                    break;
                case '\\': char_value = '\\';
                    break;
                default:
                    fail();
                    break;
                }
                consumeChar();
            }
            else
            {
                // Normal char
                char_value = currentChar();
                consumeChar();
            }
            //case like 'gh' or 'd
            if (pos >= input.size() || currentChar() != '\'')
            {
                fail();
            }
            else
            {
                consumeChar();
                Tokens.emplace_back(TypeToken::CharValue, std::string(1, char_value));
            }

            currentState = Token::Neutral;
            break;
        }
    case Token::String:
        {
            std::string string_value;

            while (pos < input.size() && currentChar() != '\"')
            {
                if (currentChar() == '\\')
                {
                    // Sequenza di escape
                    consumeChar();
                    if (pos >= input.size())
                    {
                        fail();
                        break;
                    }

                    switch (currentChar())
                    {
                    case 'n': string_value += '\n';
                        break;
                    case 't': string_value += '\t';
                        break;
                    case '\"': string_value += '\"';
                        break;
                    case '\\': string_value += '\\';
                        break;
                    default:
                        string_value += '\\';
                        string_value += currentChar();
                        break;
                    }
                    consumeChar();
                }
                else
                {
                    // Carattere normale
                    string_value += currentChar();
                    consumeChar();
                }
            }

            if (pos >= input.size())
            {
                // Stringa non chiusa
                fail();
            }
            else
            {
                consumeChar(); // Consuma il doppio apice finale "
                Tokens.emplace_back(TypeToken::StringValue, string_value);
            }

            currentState = Token::Neutral;
            break;
        }
    default:
        break;
    }
}

char Lexer::currentChar() const{
    return input[pos];
}

char Lexer::consumeChar(){
    return input[pos++];
}

TypeToken Lexer::getKeywordToken(const std::string& str)
{
    switch (hash(str.c_str()))
    {
    case hash("int"): return TypeToken::Int;
    case hash("float"): return TypeToken::Float;
    case hash("char"): return TypeToken::Char;
    case hash("string"): return TypeToken::String;
    case hash("if"): return TypeToken::If;
    case hash("else"): return TypeToken::Else;
    case hash("elif"): return TypeToken::Elif;
    case hash("while"): return TypeToken::While;
    case hash("for"): return TypeToken::For;
    case hash("return"): return TypeToken::Return;
    case hash("break"): return TypeToken::Break;
    case hash("continue"): return TypeToken::Continue;
    case hash("func"): return TypeToken::Func;
    case hash("true"): return TypeToken::True;
    case hash("false"): return TypeToken::False;
    default: return TypeToken::Identifier;
    }
}

TypeToken Lexer::getOperatorToken(const std::string& op)
{
    switch (hash(op.c_str()))
    {
    case hash("+"): return TypeToken::OpPlus;
    case hash("-"): return TypeToken::OpMinus;
    case hash("*"): return TypeToken::OpMultiply;
    case hash("/"): return TypeToken::OpDivide;
    case hash("%"): return TypeToken::OpModulo;
    case hash("="): return TypeToken::OpAssign;
    case hash("=="): return TypeToken::OpEqual;
    case hash("!="): return TypeToken::OpNotEqual;
    case hash("<"): return TypeToken::OpLess;
    case hash("<="): return TypeToken::OpLessEqual;
    case hash(">"): return TypeToken::OpGreater;
    case hash(">="): return TypeToken::OpGreaterEqual;
    case hash("&&"): return TypeToken::OpAnd;
    case hash("||"): return TypeToken::OpOr;
    case hash("!"): return TypeToken::OpNot;
    case hash("^"): return TypeToken::OpXor;
    case hash("<<"): return TypeToken::OpShiftLeft;
    case hash(">>"): return TypeToken::OpShiftRight;
    case hash("+="): return TypeToken::OpPlusAssign;
    case hash("-="): return TypeToken::OpMinusAssign;
    case hash("*="): return TypeToken::OpMultiplyAssign;
    case hash("/="): return TypeToken::OpDivideAssign;
    case hash("%="): return TypeToken::OpModuloAssign;
    case hash("("): return TypeToken::LeftParen;
    case hash(")"): return TypeToken::RightParen;
    case hash(":"): return TypeToken::Colon;
    case hash("&"):
    case hash("|"):
    default: return TypeToken::Unknown;
    }
}

bool Lexer::isOperator() const {
    static const std::string operatorChars = "=+-*/%&|<>^():!";
    return operatorChars.find(currentChar()) != std::string::npos;
}
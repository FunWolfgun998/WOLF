#include "parser/Parser.h"

Parser::Parser(std::vector<Token> tokens)
    : tokens{std::move(tokens)}, pos{0} {}


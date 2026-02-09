#ifndef WOLF_COMPILER_TOKEN_H
#define WOLF_COMPILER_TOKEN_H

#include <string>
#include "Lexer/TokenType.h"

struct Token {
    TokenType type;
    std::string lexeme; // Il testo esatto trovato nel file
    int line;
    int column;
    // Opzionale: std::any literalValue; per salvare il valore convertito (int, float)

    // Metodo per stampare il token in modo leggibile (debug)
    std::string toString() const;
};

#endif //WOLF_COMPILER_TOKEN_H
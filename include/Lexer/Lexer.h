#ifndef WOLF_COMPILER_LEXER_H
#define WOLF_COMPILER_LEXER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Token.h"

class Lexer {
public:
    // Costruttore: prende il codice sorgente intero
    explicit Lexer(std::string source);

    // Metodo principale: genera la lista di tutti i token
    std::vector<Token> tokenize();

private:
    std::string source;
    std::vector<Token> tokens;

    // Cursori per la navigazione
    size_t start = 0;   // Inizio del token corrente
    size_t current = 0; // Carattere attuale
    int line = 1;
    int column = 1;

    // Gestione Indentazione (Python Style)
    std::vector<int> indentStack; // Stack dei livelli di indentazione (0, 4, 8...)

    // Mappa per le Keywords (stringa -> tipo)
    std::unordered_map<std::string, TokenType> keywords;

    // --- Metodi Helper (Primitives) ---
    bool isAtEnd() const;
    char advance();       // Consuma e ritorna char
    char peek() const;    // Guarda char corrente
    char peekNext() const;// Guarda char successivo (Lookahead +1)
    bool match(char expected); // Se il prossimo è 'expected', consumalo

    // --- Metodi di Scansione ---
    void scanToken();     // Switch principale
    void addToken(TokenType type);
    void addToken(TokenType type, std::string text);

    // --- Gestori specifici ---
    void string();
    void number();        // Gestisce Int, Float, Hex, Bin...
    void identifier();    // Gestisce ID e Keywords
    void handleIndent();  // Gestisce la logica INDENT/DEDENT
    void skipWhitespace();
    char peekNextNext() const; // Lookahead +2 (utile per casi rari)
};

#endif //WOLF_COMPILER_LEXER_H
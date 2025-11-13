#include "parser/Parser.h" // Assicurati che il percorso sia corretto

#include <iostream>
#include <stdexcept> // Per std::runtime_error
#include <string>

// --- Costruttore ---
Parser::Parser(std::vector<Token> tokens)
    : tokens{std::move(tokens)}, pos{0} {}

void Parser::parse() {
    while (CurrentToken().type != TypeToken::End) {
        std::cout << CurrentToken().value << std::endl;
        
        advance();
    }
}
//test

Token Parser::CurrentToken() {
    if (pos >= tokens.size()) {
        // Restituisci l'ultimo token disponibile per evitare errori di accesso.
        return tokens.back();
    }
    return tokens[pos];
}

Token Parser::PeekToken(int n) {
    if (pos + n >= tokens.size()) {
        return tokens.back(); // Restituisci l'ultimo token se siamo fuori limite
    }
    return tokens[pos + n];
}

void Parser::advance() {
    if (pos < tokens.size()) {
        pos++;
    }
}

bool Parser::match(TypeToken type) {
    if (CurrentToken().type == type) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(TypeToken type, const std::string& value) {
    if (CurrentToken().type == type && CurrentToken().value == value) {
        advance();
        return true;
    }
    return false;
}

void Parser::expect(TypeToken type, std::string error_message) {
    if (CurrentToken().type != type) {
        error(error_message);
    }
    advance();
}

void Parser::error(const std::string& message) {
    Token t = CurrentToken();
    std::string errorMessage = "Errore di parsing alla riga " + std::to_string(t.line) +
                               ", colonna " + std::to_string(t.column) + ": " + message +
                               ". Trovato token di tipo '" + t.value + "'."; // Aggiungi qui una funzione per convertire TypeToken in stringa per errori migliori

    throw std::runtime_error(errorMessage);
}
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "../include/Lexer/Lexer.h" // Assicurati che il path sia corretto
#include "../include/Parser/Parser.h"
#include "../include/Utils/ASTPrinter.h"

int main(int argc, char* argv[]) {
    // 1. Controllo Argomenti
    if (argc != 2) {
        std::cerr << "Utilizzo: WOLF_Compiler <file.wlf>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    // 2. Controllo estensione (opzionale ma carino)
    if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".wlf") {
        std::cerr << "Errore: Il file deve avere estensione .wlf" << std::endl;
        return 1;
    }

    // 3. Apertura File
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Errore: Impossibile aprire il file '" << filename << "'" << std::endl;
        return 1;
    }

    // 4. Lettura intero buffer in stringa (Opzione C)
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceCode = buffer.str();
    file.close();

    // Debug: Stampa il codice letto
    std::cout << "--- LETTURA FILE WOLF ---" << std::endl;
    std::cout << sourceCode << std::endl;
    std::cout << "-------------------------" << std::endl;

    // 5. Avvio Lexer
    Lexer lexer(sourceCode);
    std::vector<Token> tokens = lexer.tokenize();

    std::cout << "\n--- TOKEN GENERATI ---\n";
    for (const auto& token : tokens) {
        std::cout << token.toString() << "\n";
    }
    Parser parser(tokens);
    try {
        std::vector<Stmt> ast = parser.parse();

        std::cout << "\n--- ABSTRACT SYNTAX TREE ---\n";
        ASTPrinter printer(std::cout);
        printer.printAST(ast); // Use the entry point we created
    } catch (const std::exception& e) {
        std::cerr << "Compilation failed due to errors.\n";
    }
    return 0;
}
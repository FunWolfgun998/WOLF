#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "../include/Lexer/Lexer.h"
#include "../include/Parser/Parser.h"
#include "../include/Utils/ASTPrinter.h"
#include "../include/Semantic/SemanticAnalyzer.h"

int main(int argc, char* argv[]) {
    // 1. Verify command-line arguments
    if (argc != 2) {
        std::cerr << "Usage: WOLF_Compiler <file.wlf>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    // 2. Validate file extension
    if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".wlf") {
        std::cerr << "Error: Input file must have .wlf extension." << std::endl;
        return 1;
    }

    // 3. Open and read source file buffer
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file '" << filename << "'." << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceCode = buffer.str();
    file.close();

    std::cout << "========================================\n";
    std::cout << "        WOLF COMPILER PIPELINE          \n";
    std::cout << "========================================\n\n";

    // 4. Lexical Analysis Phase (Tokenization)
    std::cout << "--- LEXICAL ANALYSIS ---\n";
    Lexer lexer(sourceCode);
    std::vector<Token> tokens = lexer.tokenize();

    std::cout << "Tokens successfully generated: " << tokens.size() << "\n";

    // 5. Syntax Analysis Phase (Parsing & AST Generation)
    std::cout << "\n--- SYNTACTIC ANALYSIS (PARSER) ---\n";
    Parser parser(tokens);
    std::vector<Stmt> ast;
    try {
        ast = parser.parse();
        std::cout << "AST successfully built (" << ast.size() << " top-level statements).\n";
    } catch (const std::exception& e) {
        std::cerr << "Fatal Parser error encountered. Halting.\n";
        return 1;
    }

    // 6. Semantic Analysis Phase (Type Checking, Scope & Flow Validation)
    std::cout << "\n--- SEMANTIC ANALYSIS ---\n";
    SemanticAnalyzer analyzer;
    bool semanticSuccess = analyzer.analyze(ast);

    if (!semanticSuccess) {
        std::cout << "[FAILED] Semantic analysis reported " << analyzer.getErrors().size() << " error(s):\n\n";
        for (const auto& err : analyzer.getErrors()) {
            std::cerr << "  " << err << "\n";
        }
    } else {
        std::cout << "[SUCCESS] Semantic analysis passed with 0 errors! Code is logically sound.\n";
    }

    // 7. Visual AST Tree Output
    std::cout << "\n--- ABSTRACT SYNTAX TREE VISUALIZATION ---\n";
    ASTPrinter printer(std::cout);
    printer.printAST(ast);


    // Return 0 only if both syntax and semantics were valid
    return semanticSuccess ? 0 : 1;
}
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "../include/Lexer/Lexer.h"
#include "../include/Parser/Parser.h"
#include "../include/Utils/ASTPrinter.h"
#include "../include/Semantic/SemanticAnalyzer.h"

int main(int argc, char* argv[]) {
    // Validate command-line arguments count
    if (argc != 2) {
        std::cerr << "Usage: WOLF_Compiler <file.wlf>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    // Ensure source file has the expected extension
    if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".wlf") {
        std::cerr << "Error: Input file must have .wlf extension." << std::endl;
        return 1;
    }

    // Open source file stream
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file '" << filename << "'." << std::endl;
        return 1;
    }

    // Read full source content into memory buffer
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceCode = buffer.str();
    file.close();

    std::cout << "========================================\n";
    std::cout << "        WOLF COMPILER PIPELINE          \n";
    std::cout << "========================================\n\n";

    // Stage 1: Lexical analysis (token generation)
    std::cout << "--- [STAGE 1] LEXICAL ANALYSIS ---\n";
    Lexer lexer(sourceCode);
    std::vector<Token> tokens = lexer.tokenize();

    std::cout << "Tokens successfully generated: " << tokens.size() << "\n";

    // Stage 2: Syntactic analysis (AST construction)
    std::cout << "\n--- [STAGE 2] SYNTACTIC ANALYSIS (PARSER) ---\n";
    Parser parser(tokens);
    std::vector<Stmt> ast;
    try {
        ast = parser.parse();
        std::cout << "AST successfully built (" << ast.size() << " top-level statements).\n";
    } catch (const std::exception& e) {
        std::cerr << "Fatal Parser error encountered. Halting.\n";
        return 1;
    }

    // Stage 3: Semantic analysis (type checking, scope verification, flow validation)
    std::cout << "\n--- [STAGE 3] SEMANTIC ANALYSIS ---\n";
    SemanticAnalyzer analyzer;

    // Provide source text to diagnostic engine for on-demand snippet formatting
    analyzer.setSource(filename, sourceCode);

    bool semanticSuccess = analyzer.analyze(ast);

    if (!semanticSuccess) {
        std::cout << "[FAILED] Semantic analysis reported "
                  << analyzer.getDiagnostics().errorCount() << " error(s):\n\n";

        // Print formatted rich diagnostics to standard error
        analyzer.getDiagnostics().print(std::cerr, /* ideMode = */ false);
    } else {
        std::cout << "[SUCCESS] Semantic analysis passed with 0 errors! Code is logically sound.\n";
    }

    // Stage 4: Visual AST tree output
    std::cout << "\n--- [STAGE 4] ABSTRACT SYNTAX TREE VISUALIZATION ---\n";
    ASTPrinter printer(std::cout);
    printer.printAST(ast);

    std::cout << "\n========================================\n";
    std::cout << "           COMPILATION ENDED            \n";
    std::cout << "========================================\n";

    // Exit code is 0 only when semantic validation succeeds
    return semanticSuccess ? 0 : 1;
}
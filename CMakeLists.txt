cmake_minimum_required(VERSION 3.31)
project(Wolf)

set(CMAKE_CXX_STANDARD 20)

# Aggiungi tutti i file sorgente
add_executable(
        Wolf
        src/main.cpp
        src/Lexer/Lexer.cpp
        include/Lexer/Lexer.h
        include/Lexer/TokenType.h
        include/Lexer/Token.h
        src/Lexer/Token.cpp
        include/AST/ASTdec.h
        include/AST/ASTNodes.h
        src/Parser/Parser.cpp
        include/Parser/Parser.h
        src/Utils/ASTPrinter.cpp
        include/Utils/ASTPrinter.h
        src/Semantic/Type.cpp
        include/Semantic/Type.h
        include/Semantic/Symbol.h
        include/Semantic/SymbolTable.h
        src/Semantic/SymbolTable.cpp
        include/Semantic/SemanticAnalyzer.h
        src/Semantic/SemanticAnalyzer.cpp
)

# Includi i file header
include_directories(${CMAKE_SOURCE_DIR}/include)
target_include_directories(Wolf PRIVATE include)


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
)

# Includi i file header
target_include_directories(Wolf PRIVATE include)

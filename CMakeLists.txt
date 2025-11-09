cmake_minimum_required(VERSION 3.31)
project(Wolf)

set(CMAKE_CXX_STANDARD 20)

# Aggiungi tutti i file sorgente
add_executable(
        Wolf
        src/main.cpp
        src/Lexer/Lexer.cpp
        src/Parser/Parser.cpp
        include/Parser/Parser.h
        include/Lexer/Lexer.h
        test/AST.h
        test/OLDASTVisitor.h
        test/ast_tests.cpp
        include/Lexer/Token.h
        test/parser_tests.cpp
        test/ASTPrinter.h
        src/AST/ASTPrinter.cpp
        include/Parser/ParseError.h
        test/ASTNode.h
        include/AST/Nodes/BinaryExprNode.h
        include/AST/Nodes/ProgramNode.h
        include/AST/Nodes/IntLiteralNode.h
        include/AST/Nodes/StringLiteralNode.h
        include/AST/Nodes/BootLiteralNode.h
        include/AST/Nodes/NullLiteralNode.h
        include/AST/Nodes/IdentifierNode.h
        include/AST/Nodes/AssignmentNode.h
        include/AST/Nodes/UnaryExprNode.h
        include/AST/Nodes/VariableDeclNode.h
        include/AST/Nodes/AllNodes.h
        include/AST/Nodes/VariableInitNode.h
        include/AST/Nodes/CharLiteralNode.h
        src/AST/ASTNode.cpp
        include/AST/Nodes/IfNode.h
        include/AST/Nodes/BlockNode.h
        include/Lexer/StateToken.h
        include/Parser/Scope.h
        include/Parser/Symbol.h
        include/AST/AST.h
        include/AST/Visitor.h
        include/AST/NodeType.h
        include/AST/ASTNote.h
)

# Includi i file header
target_include_directories(Wolf PRIVATE include)

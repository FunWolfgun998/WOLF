#ifndef WOLF_COMPILER_TOKENTYPE_H
#define WOLF_COMPILER_TOKENTYPE_H

enum class TokenType {
    // Speciali
    END_OF_FILE, ERROR, UNKNOWN,

    // Struttura
    NEWLINE,      // \n
    INDENT,       // Aumento rientro
    DEDENT,       // Diminuzione rientro

    // Letterali
    INT_LITERAL,    // 10
    FLOAT_LITERAL,  // 10.5
    HEX_LITERAL,    // 0xFF
    BIN_LITERAL,    // 0b101
    OCT_LITERAL,    // 0o77
    CHAR_LITERAL,   // 'c'
    STRING_LITERAL, // "ciao"
    RAW_STRING_LITERAL, // r""
    FORMAT_STRING_LITERAL, // f""

    // Keywords (Tipi)
    KW_INT, KW_FLOAT, KW_CHAR, KW_STRING, KW_VOID, KW_BOOL,
    KW_PUBLIC, KW_PRIVATE, KW_PROTECTED, KW_THIS, KW_EXTENDS,

    // Keywords (Controllo Flusso)
    KW_IF, KW_ELSE, KW_ELIF,
    KW_WHILE, KW_FOR, KW_IN,
    KW_RETURN, KW_BREAK, KW_CONTINUE,
    KW_STRUCT, KW_CLASS,
    KW_NULL, KW_TRUE, KW_FALSE,


    // Operatori Doppi (Priorità Alta)
    OP_SHIFT_LEFT,  // <<
    OP_SHIFT_RIGHT, // >>
    OP_SHIFT_LEFT_ASS,  // <<=
    OP_SHIFT_RIGHT_ASS, // >>=
    OP_EQ_EQ,       // ==
    OP_NOT_EQ,      // !=
    OP_LESS_EQ,     // <=
    OP_GRT_EQ,      // >=
    OP_AND,         // &&
    OP_OR,          // ||
    OP_PLUS_ASS,    // +=
    OP_MIN_ASS,     // -=
    OP_MUL_ASS,     // *=
    OP_DIV_ASS,     // /=
    OP_MOD_ASS,     // %=
    OP_AND_ASS,     // &=
    OP_OR_ASS,      // |=
    OP_XOR_ASS,     // ^=
    RANGE_OP,       // ..
    ARROW,          // -> (Opzionale, utile per return type)

    // Operatori Singoli
    OP_BIT_AND,     // &
    OP_BIT_OR,      // |
    OP_PLUS,        // +
    OP_MINUS,       // -
    OP_STAR,        // *
    OP_SLASH,       // /
    OP_MOD,         // %
    OP_XOR,         // ^
    OP_ASSIGN,      // =
    OP_NOT,         // !
    OP_LESS,        // <
    OP_GRT,         // >
    OP_TILDE,    // ~
    OP_QUESTION, // ?
    OP_INC,      // ++
    OP_DEC,      // --

    // Punteggiatura
    DOT,            // .
    COLON,          // :
    L_PAREN,        // (
    R_PAREN,        // )
    L_BRACKET,      // [
    R_BRACKET,      // ]
    COMMA,          // ,

    // Identificatori
    IDENTIFIER      // myVar
};

#endif //WOLF_COMPILER_TOKENTYPE_H
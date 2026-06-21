| Indice | Enum Token | Esempio / Descrizione |
| :---: | :--- | :--- |
| **Speciali** | | |
| 0 | `END_OF_FILE` | Fine del file (EOF) |
| 1 | `ERROR` | Token di errore |
| 2 | `UNKNOWN` | Token sconosciuto |
| **Struttura** | | |
| 3 | `NEWLINE` | `\n` (A capo) |
| 4 | `INDENT` | Aumento rientro (inizio blocco) |
| 5 | `DEDENT` | Diminuzione rientro (fine blocco) |
| **Letterali** | | |
| 6 | `INT_LITERAL` | `10`, `42` |
| 7 | `FLOAT_LITERAL` | `10.5`, `3.14` |
| 8 | `HEX_LITERAL` | `0xFF`, `0xA1` |
| 9 | `BIN_LITERAL` | `0b101`, `0b1100` |
| 10 | `OCT_LITERAL` | `0o77`, `0o12` |
| 11 | `CHAR_LITERAL` | `'c'`, `'X'` |
| 12 | `STRING_LITERAL` | `"ciao"`, `"Wolf Lang"` |
| **Keywords (Tipi)** | | |
| 13 | `KW_INT` | `int` |
| 14 | `KW_FLOAT` | `float` |
| 15 | `KW_CHAR` | `char` |
| 16 | `KW_STRING` | `string` |
| 17 | `KW_VOID` | `void` |
| 18 | `KW_BOOL` | `bool` |
| **Keywords (Controllo)** | | |
| 19 | `KW_IF` | `if` |
| 20 | `KW_ELSE` | `else` |
| 21 | `KW_ELIF` | `elif` |
| 22 | `KW_WHILE` | `while` |
| 23 | `KW_FOR` | `for` |
| 24 | `KW_IN` | `in` |
| 25 | `KW_RETURN` | `return` |
| 26 | `KW_BREAK` | `break` |
| 27 | `KW_CONTINUE` | `continue` |
| 28 | `KW_STRUCT` | `struct` |
| 29 | `KW_CLASS` | `class` |
| 30 | `KW_NULL` | `null` |
| 31 | `KW_TRUE` | `true` |
| 32 | `KW_FALSE` | `false` |
| **Operatori Doppi** | | |
| 33 | `OP_SHIFT_LEFT` | `<<` |
| 34 | `OP_SHIFT_RIGHT` | `>>` |
| 35 | `OP_EQ_EQ` | `==` |
| 36 | `OP_NOT_EQ` | `!=` |
| 37 | `OP_LESS_EQ` | `<=` |
| 38 | `OP_GRT_EQ` | `>=` |
| 39 | `OP_AND` | `&&` |
| 40 | `OP_OR` | `\|\|` |
| 41 | `OP_PLUS_ASS` | `+=` |
| 42 | `OP_MIN_ASS` | `-=` |
| 43 | `OP_MUL_ASS` | `*=` |
| 44 | `OP_DIV_ASS` | `/=` |
| 45 | `OP_MOD_ASS` | `%=` |
| 46 | `RANGE_OP` | `..` |
| 47 | `ARROW` | `->` |
| **Operatori Singoli** | | |
| 48 | `OP_PLUS` | `+` |
| 49 | `OP_MINUS` | `-` |
| 50 | `OP_STAR` | `*` |
| 51 | `OP_SLASH` | `/` |
| 52 | `OP_MOD` | `%` |
| 53 | `OP_XOR` | `^` |
| 54 | `OP_ASSIGN` | `=` |
| 55 | `OP_NOT` | `!` |
| 56 | `OP_LESS` | `<` |
| 57 | `OP_GRT` | `>` |
| **Punteggiatura** | | |
| 58 | `DOT` | `.` |
| 59 | `COLON` | `:` |
| 60 | `L_PAREN` | `(` |
| 61 | `R_PAREN` | `)` |
| 62 | `COMMA` | `,` |
| **Identificatori** | | |
| 63 | `IDENTIFIER` | `myVar`, `functionName` |

Ti allego ora i file sorgente crea memorie riguardo il progetto,
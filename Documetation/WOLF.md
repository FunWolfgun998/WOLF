# Project Architecture

The **WOLF Compiler** is an ahead-of-time (AOT) language frontend engineered in modern C++ (C++17/C++20). It translates programs written in the **WOLF** programming language (`.wlf`) through an explicit, decoupled, compilation pipeline. The architecture is intentionally free of third-party dependencies, relying solely on the C++ Standard Library to ensure deterministic compilation, portability, and zero-cost abstraction patterns.

---

## General Overview & Philosophy of the WOLF Language

The WOLF programming language is designed to reconcile two historically competing language paradigms:
1. **The visual ergonomics and structural cleanliness of off-side rule languages** (Python-style block structure delimited by significant whitespace, indentation, and dedentation).
2. **The safety, predictability, and performance of strictly typed, nominal systems** (C++, Rust, and C#-style type discipline, explicit memory invariants, and compile-time verification).

### Key Architectural & Language Tenets

- **Significant Whitespace without Semantic Ambiguity**: Code blocks are syntactically bounded by physical indentation changes (`INDENT` and `DEDENT` tokens). Colons (`:`) terminate control-flow and declaration headers, establishing unambiguous block entries without enclosing braces (`{}`).
- **Strict Nominal Type System**: Every expression and variable resolves to a statically known type. Type safety is verified before any target lowering occurs. The language distinguishes between stack-allocated/aggregate value layouts (`struct`) and reference/polymorphic OOP entities (`class`).
- **First-Class Null Safety**: Types are non-nullable by default. Nullable variants require an explicit suffix modifier `?` (e.g., `int?`, `Dog?`, `int[]?`). The type checker enforces compile-time null verification: dereferencing or indexing a nullable entity without a guarding assertion is a breaking diagnostic (`E0014`, `E0015`).
- **Control-Flow-Sensitive Smart-Casting**: Conditionals that perform explicit null checks (e.g., `if (val != null):`) locally promote nullable variables to their non-nullable counterparts within the guarded branch scope. Outer scopes automatically regain the original nullable type signature once the block exits.
- **Definite Assignment & Immutability**: Variables cannot be evaluated before explicit initialization (`E0002`). Variables qualified with `const` cannot be reassigned after declaration (`E0003`).
- **Guaranteed Path Termination**: Non-void subroutines are statically checked for return-path completeness (`hasGuaranteedReturn`). Functions that fail to yield a value along all execution branches trigger fatal diagnostics (`E0025`), while unreachable statements following terminating jumps trigger non-breaking warnings (`W0003`).

---

## Compiler Pipeline

The compilation pipeline is implemented as a unidirectional, fail-fast dataflow where each stage processes the validated artifact produced by its predecessor.

```
┌───────────────────────┐
│    [STAGE 1] Lexer    │ ◄─── Significant whitespace, FNV-1a hashing,
└───────────┬───────────┘ UTF-8 decoder, string interning
            │ Token Stream
            ▼
┌───────────────────────┐
│   [STAGE 2] Parser    │ ◄─── Hybrid: Recursive Descent (Statements) +
└───────────┬───────────┘ Pratt Parsing (Precedence climbing for Exprs)
            │
      AST   │         AST        ┌─────────────────────────┐
            │ ───────────────────│ [STAGE 2.5] AST Printer │ ◄─── Visual tree dump for Parser debugging
            │                    └─────────────────────────┘ and structural verification
            ▼            
┌───────────────────────┐
│  [STAGE 3] Semantic   │ ◄─── Pass 1: Declaration & Layout Registration
│       Analyzer        │ ◄─── Pass 2: Type Checking, Definite Assignment,
└───────────┬───────────┘ Smart Casts & Rust-grade Diagnostics
            │
            │ Validated & Annotated AST
            │
            ▼
```

### Stage 1: Lexical Analysis (`Lexer`)
- **Input**: Raw UTF-8 source string buffer (`std::string`).
- **Output**: Linear stream of strongly typed tokens (`std::vector<Token>`).
- **Operational Mechanics**:
    - Operates via a single-pass character-consumption model with an internal cursor (`current`, `start`) and two-dimensional source coordinate tracking (`line`, `column`).
    - Implements an explicit indentation stack (`indentStack`) that tracks physical column depths. On physical newlines (`\n`), the scanner counts leading whitespace (expanding tabs to 4 spaces) and synthesizes `INDENT` and `DEDENT` tokens to translate layout geometry into discrete syntax boundaries.
    - Multi-base numeric literal parsing: Evaluates hexadecimal (`0x`), binary (`0b`), octal (`0o`), and IEEE 754 floating-point notations.
    - Compile-time string interning and token-type matching: Keywords are resolved in $O(1)$ time using a compile-time FNV-1a 32-bit hash algorithm evaluated via `switch` constructs.
    - Supports string literals (`"..."`), raw strings (`r"..."`), format/interpolated strings (`f"..."`), and multi-byte UTF-8 character literal decoding into 32-bit scalar values.

### Stage 2: Syntactic Analysis (`Parser`)
- **Input**: Linear token stream (`std::vector<Token>`).
- **Output**: Top-level Abstract Syntax Tree (`std::vector<Stmt>`).
- **Operational Mechanics**:
    - Implements a hybrid parsing architecture:
        - **Recursive Descent** handles top-level declarations, classes, structs, loops (`while`, `for`), conditionals (`if`, `elif`, `else`), and scoped block boundaries (`parseBlock`).
        - **Pratt Parsing (Top-Down Operator Precedence)** parses complex binary, unary, postfix, range, ternary, cast, and call expressions through explicit Null Denotation (`Nud`) and Left Denotation (`Led`) function dispatch tables.
    - Precedence levels (`Precedence` enum) resolve operator binding power unambiguously, handling left-associativity and ternary chaining.
    - Employs **Panic Mode** error synchronization (`synchronize`): upon encountering a malformed statement, the parser consumes tokens until hitting statement terminators (`NEWLINE`) or structural keywords (`class`, `struct`, `if`, etc.), allowing the compiler to report multiple syntax errors without cascading.


### Stage 2.5: Visual AST Tree Output (`ASTPrinter`)
- **Input**: Abstract Syntax Tree statements (`const std::vector<Stmt>&`) emitted by the Parser.
- **Output**: Streamed Unicode box-drawing visualization (`├──`, `└──`, `│`).
- **Operational Mechanics**:
    - Visits nodes via `std::visit` and computes structural branch prefixes dynamically (`currentPrefix`).
    - Provides inspection of AST topology, node variants, expression associativity, and statement nesting for compiler verification and debugging.

---
### Stage 3: Semantic Analysis & Type System (`SemanticAnalyzer`)
- **Input**: Raw AST (`std::vector<Stmt>`).
- **Output**: Boolean validation status + rich diagnostic stream (`DiagnosticEngine`).
- **Operational Mechanics**:
    - Implemented as a **Two-Pass Visitor** using `std::visit` across closed sum-types (`ExprVariant`, `StmtVariant`):
        - **Pass 1 (`collectDeclarations`)**: Traverses top-level nodes to register classes, structs, fields, and method signatures into the `SymbolTable`. Resolves inheritance hierarchies and executes cycle-detection checks (`E0035`). This guarantees that types and methods can be referenced out-of-order and mutually depend on each other.
        - **Pass 2 (`typeCheck`)**: Traverses statement bodies and expressions to verify type compatibility, enforce accessibility modifiers (`public`, `private`, `protected`), perform definite assignment checks, calculate branch return completeness, and apply local non-null type promotions.
    - Integrates an advanced **Diagnostic Engine** supporting multi-span reporting, terminal colorization (ANSI), line pre-indexing, and Levenshtein-distance string similarity calculations for typo correction ("did you mean?").


## Engineering Guidelines & Architectural Standards

To maintain uniformity, mechanical sympathy, and maintainability across the codebase, all subsystems adhere to the following architectural conventions.

### Memory Layout & Ownership Semantics
- **Exclusive Tree Ownership**: AST nodes must always be managed through `std::unique_ptr<T>`. Shared ownership of syntax nodes is prohibited.
- **Variant-Based Sum Types**: Polymorphic syntax nodes do not rely on traditional virtual table hierarchies (`vtable`). Instead, nodes are modeled as variants (`ExprVariant`, `StmtVariant`) wrapped in value containers (`Expr`, `Stmt`). Nodes are traversed using `std::visit`, yielding contiguous memory layout, cache locality, and exhaustive compile-time pattern matching.
- **Flyweight Type Pointers**: `Type` objects are immutable once constructed and must never be duplicated arbitrarily. Primitive types are accessed via static singletons (`Type::getInt()`, `Type::getFloat()`, etc.). Complex types (arrays, functions, nullables) are interned and owned by dynamic memory pools (`typePool`, `customTypes`), and referred to throughout the compiler via non-owning raw pointers (`const Type*`). Pointer equality (`typeA == typeB`) serves as the primary fast-path identity check.

### Error Propagation & Diagnostics Architecture
- **No Direct Standard Stream Dumping in Subsystems**: Core compiler subsystems (`Lexer`, `Parser`, `SemanticAnalyzer`) must never write diagnostics directly to `std::cout` or `std::cerr` (with the exception of development-level panic assertions).
- **Centralized Diagnostic Engine**: All compilation errors, warnings, and notes must be routed through `DiagnosticEngine::report`, `reportWarning`, or `reportMultiSpan`.
- **Source Coordinate Anchoring**: Every reported diagnostic must be anchored to an explicit token or a computed `SourceSpan` (line, column, character length).
- **Encountered Error Sentinel**: Expressions that fail type analysis must immediately return `Type::getError()`. Visitor logic must check `type->isError()` and gracefully short-circuit to suppress error cascades.

### Naming Conventions
- **Types, Classes, Structs, and Enums**: `PascalCase`
    - *Examples*: `Lexer`, `DiagnosticEngine`, `BinaryExpr`, `AccessModifier`, `TokenType`.
- **Methods and Functions**: `camelCase`
    - *Examples*: `tokenize()`, `parsePrecedence()`, `resolveTypeFromToken()`, `reportMultiSpan()`.
- **Member Variables & Fields**: `camelCase`
    - Private member fields do not use m-prefixes or trailing underscores; they rely on semantic clarity and scoped member access.
    - *Examples*: `source`, `currentScope`, `lineOffsets`, `isNullable`.
- **Constants & Enumeration Values**: `UPPER_SNAKE_CASE`
    - *Examples*: `TokenType::INT_LITERAL`, `DiagnosticCode::E0001_UNDECLARED_VARIABLE`, `ScopeKind::GLOBAL`.

### File & Directory Organization
- **Separation of Interface and Implementation**:
    - Declarations and inline templates reside under `include/<Subsystem>/<Name>.h`.
    - Concrete implementation logic resides under `src/<Subsystem>/<Name>.cpp`.
- **Header Guards**: All headers must use `#pragma once` at line 1.
- **Dependency Minimization**: Forward declarations must be placed in dedicated declaration headers (such as `ASTdec.h`) to decouple header inclusions and prevent circular translation-unit dependencies. Standard library includes must be kept minimal within header files (using `<iosfwd>` where full streams are not required).
---
# Lexer - Lexical Analysis

The Lexer subsystem is responsible for transforming a raw UTF-8 input character stream into a discrete, strongly-typed sequence of tokens (`std::vector<Token>`). It executes the first phase of the WOLF compiler pipeline and establishes the syntactic ground truth for downstream ingestion by the Parser.

### Operational Pipeline & Consumption Model
The Lexer operates as a linear, single-pass scanner that inspects the source string through a sliding cursor model defined by two byte offsets: `start` (the beginning of the token currently being evaluated) and `current` (the lookahead scan head). Source coordinates (`line`, `column`, and `startColumn`) are updated monotonically as characters are consumed.

```
 Source Stream: [ f ] [ " ] [ H ] [ e ] [ l ] [ l ] [ o ] [ " ]
                 ▲           ▲
                 │           └── current (Lookahead Cursor)
                 └────────────── start   (Token Anchor)
```

### Indentation-Aware Scanning Strategy (Off-Side Rule)
Unlike brace-delimited languages, WOLF enforces visual scoping via an indentation stack (`indentStack`). The Lexer integrates an indentation handler that triggers on physical newline characters (`\n`):
1. **Lookahead Blank/Comment Filtration**: When a newline is encountered, the scanner uses a non-committing cursor to count physical leading whitespace (where tabs `\t` are expanded to 4 spaces). If the line contains only whitespace or single/multiline comments, the entire indentation check is discarded without altering the indentation stack or emitting tokens.
2. **Block Entry (`INDENT`)**: If the validated indentation column count exceeds the top of `indentStack`, the new column depth is pushed onto the stack, and a synthetic `TokenType::INDENT` token is emitted.
3. **Block Exit (`DEDENT`)**: If the indentation column count is strictly less than the top of `indentStack`, the stack is repeatedly popped, synthesizing a `TokenType::DEDENT` token for each closed lexical scope until the matching indentation level is reached. If the dedented column count does not match any existing level on the stack, an indentation inconsistency error is reported.
4. **Stream Termination Synthesis**: Upon reaching the end of the input stream, the Lexer guarantees structural closure by emitting a terminal `NEWLINE` (if the trailing token requires termination), popping all remaining indentation frames to emit trailing `DEDENT` tokens, and appending a terminal `END_OF_FILE` token.

### Maximal Munch & Lookahead Buffering
The scanning loop applies the principle of Maximal Munch (longest match preference) using bounded character lookahead functions (`peek`, `peekNext`). This mechanism resolves lexical ambiguities without backtracking:
- **Dot vs. Range vs. Floating-Point**: Distinguishes between property access (`.`), range operators (`..`), and floating-point literals beginning with a dot (`.5`) or preceded by an integer (`10.toString()` vs. `10.5` vs. `10..20`).
- **Compound Operators**: Distinguishes between single-character operators, double-character operators, compound assignments, and 3-character shift assignments (`<`, `<=`, `<<`, `<<=`).
- **Nested Multiline Comments**: Implements stateful comment parsing supporting arbitrary recursion depths (`/* ... /* ... */ ... */`).

### Identifier Interning & Static Hashing
Identifiers and keywords are extracted and categorized via compile-time FNV-1a 32-bit hashing (`hash()`), enabling $O(1)$ switch-case branch execution. Identifiers that do not match reserved keywords are assigned a dense integer identifier via an internal string interning map (`identifierIDs`), optimizing subsequent symbol lookups and equality checks.

---

## Enum Class: `TokenType`

Defines the complete lexical taxonomy recognized by the WOLF compiler front-end.

- **Underlying Type**: Implicit `int`.
- **Structural Invariant**: Every token emitted by the Lexer must have a valid `TokenType` tag within this enumeration.
- **Categorization**:
    - **Special**: `END_OF_FILE`, `ERROR`, `UNKNOWN`.
    - **Structural (Off-Side Layout)**: `NEWLINE`, `INDENT`, `DEDENT`.
    - **Numeric & Scalar Literals**: `INT_LITERAL`, `FLOAT_LITERAL`, `HEX_LITERAL`, `BIN_LITERAL`, `OCT_LITERAL`, `CHAR_LITERAL`, `STRING_LITERAL`, `RAW_STRING_LITERAL`, `FORMAT_STRING_LITERAL`.
    - **Keywords (Types & Modifiers)**: `KW_INT`, `KW_FLOAT`, `KW_CHAR`, `KW_STRING`, `KW_VOID`, `KW_BOOL`, `KW_PUBLIC`, `KW_PRIVATE`, `KW_PROTECTED`, `KW_THIS`, `KW_BASE`, `KW_EXTENDS`, `KW_STATIC`, `KW_ABSTRACT`, `KW_OVERRIDE`, `KW_CONST`, `KW_IMPORT`, `KW_EXTERN`.
    - **Keywords (Control Flow & Logic)**: `KW_IF`, `KW_ELSE`, `KW_ELIF`, `KW_WHILE`, `KW_FOR`, `KW_IN`, `KW_MATCH`, `KW_CASE`, `KW_DEFAULT`, `KW_RETURN`, `KW_BREAK`, `KW_CONTINUE`, `KW_STRUCT`, `KW_CLASS`, `KW_ENUM`, `KW_AS`, `KW_IS`, `KW_NULL`, `KW_TRUE`, `KW_FALSE`.
    - **Operators (Arithmetic, Bitwise, Logical, Assignment)**: `OP_PLUS`, `OP_MINUS`, `OP_STAR`, `OP_SLASH`, `OP_MOD`, `OP_XOR`, `OP_BIT_AND`, `OP_BIT_OR`, `OP_TILDE`, `OP_ASSIGN`, `OP_NOT`, `OP_LESS`, `OP_GRT`, `OP_QUESTION`, `OP_INC`, `OP_DEC`, `OP_SHIFT_LEFT`, `OP_SHIFT_RIGHT`, `OP_EQ_EQ`, `OP_NOT_EQ`, `OP_LESS_EQ`, `OP_GRT_EQ`, `OP_AND`, `OP_OR`, `OP_PLUS_ASS`, `OP_MIN_ASS`, `OP_MUL_ASS`, `OP_DIV_ASS`, `OP_MOD_ASS`, `OP_AND_ASS`, `OP_OR_ASS`, `OP_XOR_ASS`, `OP_SHIFT_LEFT_ASS`, `OP_SHIFT_RIGHT_ASS`, `RANGE_OP` (`..`), `ARROW` (`->`).
    - **Punctuation & Syntax**: `DOT`, `COLON`, `L_PAREN`, `R_PAREN`, `L_BRACKET`, `R_BRACKET`, `COMMA`.
    - **User Identifiers**: `IDENTIFIER`.

---

## Alias: `LiteralValue`

- **Definition**:
  ```cpp
  using LiteralValue = std::variant<std::monostate, long long, double, std::string, char, uint32_t>;
  ```
- **Structural Invariant**: Represents a type-safe, closed sum type for literal payloads embedded directly inside a `Token`.
- **Memory Footprint**: Sized to the largest alternative (`std::string`, typically 24–32 bytes) plus discriminant overhead and alignment padding (~32–40 bytes total).
- **Ownership Semantics**: Value-semantic storage. String variants manage their own internal heap buffer. Default-constructed tokens hold `std::monostate`.

---

## Struct: `Token`

Represents an immutable, discrete lexical unit emitted by the `Lexer` and consumed by the `Parser`.

- **Structural Invariant**: A `Token` encapsulates its syntactic classification, exact source text slice, 1-based source coordinates, an optional interned identifier ID, and an optional literal value variant.
- **Memory Footprint**: Approximately 80–96 bytes depending on ABI padding: `TokenType` (4 bytes), `std::string lexeme` (24–32 bytes), `int line` (4 bytes), `int column` (4 bytes), `LiteralValue literalValue` (32–40 bytes), `int id` (4 bytes).
- **Lifecycle & Ownership**: Value-semantic type. Supports standard copy, move construction, and assignment.

### Member Fields
- `TokenType type`: The grammatical category of the token.
- `std::string lexeme`: The exact raw substring extracted from the source file.
- `int line`: The 1-based line number where the token starts.
- `int column`: The 1-based column offset where the token starts.
- `LiteralValue literalValue`: The parsed runtime value for literal tokens; contains `std::monostate` for non-literal tokens.
- `int id`: Dense interned integer assigned to `IDENTIFIER` tokens (0 for keywords/symbols).

### Constructors & Methods

### `Token()`
- **Signature**: `Token()`
- **Parameters & Fields**: None. Sets `type` to `TokenType::UNKNOWN`, `lexeme` to `""`, `line` to `0`, `column` to `0`, `literalValue` to `std::monostate{}`, and `id` to `0`.
- **Technical Purpose & Mechanics**: Acts as the default constructor, creating an uninitialized sentinel token.
- **Dependencies & Invocations**: Used in container pre-allocations and fallback/sentinel branches across Parser and Semantic phases.

### `Token(TokenType, std::string, int, int)`
- **Signature**: `Token(TokenType type, std::string lexeme, int line, int col)`
- **Parameters & Fields**:
    - `type`: Target `TokenType`.
    - `lexeme`: Rvalue or lvalue string representation.
    - `line`: 1-based line index.
    - `col`: 1-based column index.
- **Technical Purpose & Mechanics**: Constructs a standard structural token, operator, keyword, or punctuation mark without an interned ID or literal payload. Moves `lexeme` into member storage.
- **Dependencies & Invocations**: Invoked directly by `Lexer::addToken(TokenType)`.

### `Token(TokenType, std::string, int, int, int)`
- **Signature**: `Token(TokenType type, std::string lexeme, int line, int col, int id)`
- **Parameters & Fields**:
    - `type`: Target `TokenType` (typically `TokenType::IDENTIFIER`).
    - `lexeme`: Identifier string.
    - `line`: 1-based line index.
    - `col`: 1-based column index.
    - `id`: Dense interned integer ID (`>= 1`).
- **Technical Purpose & Mechanics**: Initializes an identifier token carrying its string-interned unique ID.
- **Dependencies & Invocations**: Invoked exclusively by `Lexer::addIdentifierToken(TokenType, int)`.

### `Token(TokenType, std::string, int, int, LiteralValue)`
- **Signature**: `Token(TokenType type, std::string lexeme, int line, int col, LiteralValue value)`
- **Parameters & Fields**:
    - `type`: Target literal `TokenType`.
    - `lexeme`: Original source slice.
    - `line`: 1-based line index.
    - `col`: 1-based column index.
    - `value`: Parsed `LiteralValue` variant moved into the token.
- **Technical Purpose & Mechanics**: Constructs a literal token containing both the raw text representation and its parsed native value.
- **Dependencies & Invocations**: Invoked by `Lexer::addToken(TokenType, LiteralValue)`.

### `asInt()`
- **Signature**: `long long asInt() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Inspects `literalValue` using `std::get_if<long long>`. If active, returns the stored 64-bit integer; otherwise, returns `0`.
- **Dependencies & Invocations**: Called during expression evaluation and constant folding in AST/Semantic phases.

### `asDouble()`
- **Signature**: `double asDouble() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Inspects `literalValue` using `std::get_if<double>`. If active, returns the IEEE 754 double precision float; otherwise, returns `0.0`.
- **Dependencies & Invocations**: Called during semantic analysis of numeric expressions.

### `asString()`
- **Signature**: `std::string asString() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Inspects `literalValue` using `std::get_if<std::string>`. If active, returns a copy of the stored string; otherwise, returns an empty string `""`.
- **Dependencies & Invocations**: Invoked by `Parser::parseFormatString()` to retrieve raw interpolated string templates.

### `asChar()`
- **Signature**: `char asChar() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Inspects `literalValue` using `std::get_if<char>`. If active, returns the stored ASCII/char scalar; otherwise, returns `'\0'`.
- **Dependencies & Invocations**: Used in character literal validations.

### `toString()`
- **Signature**: `std::string toString() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Generates a detailed debug diagnostic string containing token type name, escaped lexeme, source coordinates, interned ID (if non-zero), and extracted variant value.
- **Dependencies & Invocations**: Invokes `tokenTypeToString(type)`. Used in compiler tracing and diagnostics.

---

## Standalone Function: `tokenTypeToString`

- **Signature**: `std::string tokenTypeToString(TokenType type)`
- **Parameters & Fields**:
    - `type`: The `TokenType` enumeration value to inspect.
- **Technical Purpose & Mechanics**: Executes an exhaustive `switch` over all defined `TokenType` values, returning the canonical string representation (e.g., `"INT_LITERAL"`, `"KW_CLASS"`, `"OP_PLUS"`). Falls back to `"UNKNOWN"`.
- **Dependencies & Invocations**: Called by `Token::toString()`.

---

## Standalone Function: `hash`

- **Signature**: `constexpr uint32_t hash(const char* str, uint32_t h = 0x811c9dc5)`
- **Parameters & Fields**:
    - `str`: Null-terminated string pointer to hash. Must not be null.
    - `h`: Seed accumulator initialized to the FNV-1a 32-bit offset basis (`0x811c9dc5`).
- **Technical Purpose & Mechanics**: Recursively evaluates the 32-bit Fowler–Noll–Vo (FNV-1a) hash algorithm at compile time:
  $$h \leftarrow (h \oplus \text{character}) \times 16777619$$
  Terminates upon encountering the null byte `\0`. Enables compile-time string hashing for constant expressions inside `switch` statements, avoiding runtime string comparison overhead.
- **Dependencies & Invocations**: Invoked within `Lexer::identifier()` to match keyword candidates in $O(1)$ time.

---

## Class: `Lexer`

The core scanning engine that consumes source code and generates the token sequence.

- **Structural Invariant**:
    - `source` holds an immutable copy of the input source text.
    - `start <= current <= source.length()`.
    - `indentStack` is guaranteed to contain at least one element (`0` at base level) throughout execution.
- **Memory Footprint**: Holds `std::string source` (24–32 bytes), `std::vector<Token> tokens` (24 bytes), scanning pointers/coordinates (32 bytes), `std::vector<int> indentStack` (24 bytes), `std::unordered_map<std::string, int> identifierIDs` (~56 bytes), plus map node allocations.
- **Lifecycle & Ownership**: Owns its internal token buffer and manages memory allocations for string interning.

### Member Fields
- `std::string source`: Immutable source code buffer.
- `std::vector<Token> tokens`: Vector of tokens accumulated during scanning.
- `size_t start`: Byte offset pointing to the start of the current token.
- `size_t current`: Byte offset pointing to the current scan head.
- `int line`: Current line number (1-based).
- `int column`: Current column number (1-based).
- `int startColumn`: Column coordinate where the current token began.
- `std::vector<int> indentStack`: Stack tracking physical indentation levels (in space equivalents).
- `std::unordered_map<std::string, int> identifierIDs`: Interning dictionary mapping identifier names to unique integers.
- `int nextID`: Monotonically increasing counter for assigning interned IDs (starts at 1).

### Constructors & Methods

### `Lexer(std::string)`
- **Signature**: `explicit Lexer(std::string source)`
- **Parameters & Fields**:
    - `source`: Rvalue or lvalue string containing the complete input program.
- **Technical Purpose & Mechanics**: Moves the input string into `this->source` and initializes `indentStack` by pushing baseline level `0`.
- **Dependencies & Invocations**: Called by `main.cpp` and `Parser::parseFormatString()`.

### `tokenize()`
- **Signature**: `std::vector<Token> tokenize()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Executes the top-level lexical scanning loop:
    1. Repeatedly sets `start = current` and `startColumn = column`, invoking `scanToken()` until `isAtEnd()` evaluates to true.
    2. If the last emitted token is neither `NEWLINE` nor `DEDENT`, appends a synthetic `TokenType::NEWLINE`.
    3. Unwinds `indentStack` while `indentStack.back() > 0`, popping levels and emitting synthetic `TokenType::DEDENT` tokens.
    4. Appends a final `TokenType::END_OF_FILE` token.
    5. Returns the populated `tokens` vector by value (invoking NRVO or move semantics).
- **Dependencies & Invocations**: Invokes `isAtEnd()`, `scanToken()`, and pushes to `tokens`.

### `isAtEnd()`
- **Signature**: `bool isAtEnd() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns `true` if `current >= source.length()`, indicating that the character stream has been exhausted.
- **Dependencies & Invocations**: Called throughout `tokenize()`, `advance()`, `peek()`, `match()`, and comment/string handlers.

### `advance()`
- **Signature**: `char advance()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns `'\0'` if `isAtEnd()` is true. Otherwise, increments `column` by 1 and returns `source[current++]`.
- **Dependencies & Invocations**: Core primitive consumed by all token scanning routines.

### `peek()`
- **Signature**: `char peek() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns `'\0'` if `isAtEnd()` is true; otherwise, returns the character at `source[current]` without advancing the cursor.
- **Dependencies & Invocations**: Used for lookahead decisions throughout the scanner.

### `peekNext(int)`
- **Signature**: `char peekNext(int n = 1) const`
- **Parameters & Fields**:
    - `n`: Forward lookahead offset (defaults to 1).
- **Technical Purpose & Mechanics**: Checks if `current + n >= source.length()`. If out of bounds, returns `'\0'`; otherwise, returns `source[current + n]`.
- **Dependencies & Invocations**: Consumed by comment nesting lookaheads, numeric base detection, and operator matching.

### `match(char)`
- **Signature**: `bool match(char expected)`
- **Parameters & Fields**:
    - `expected`: Character to match against the current scan head.
- **Technical Purpose & Mechanics**: If `isAtEnd()` is true or `source[current] != expected`, returns `false`. Otherwise, increments `current` and `column` (via `advance()`) and returns `true`.
- **Dependencies & Invocations**: Invokes `isAtEnd()` and `advance()`. Used extensively in compound operator identification.

### `scanToken()`
- **Signature**: `void scanToken()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Consumes the next character via `advance()` and executes a dispatch `switch`:
    - Single characters (`(`, `)`, `,`, `:`, `[`, `]`, `~`, `?`): Dispatches corresponding token types directly.
    - Multi-character operators (`+`, `-`, `*`, `/`, `%`, `^`, `=`, `!`, `<`, `>`, `&`, `|`): Resolves maximal munch sequences via `match()`.
    - Dot (`.`): Dispatches `RANGE_OP` (`..`), triggers `number()` if followed by a digit, or emits `DOT`.
    - Slash (`/`): Handles line comments (`//`), stateful nested multiline comments (`/* ... */`), division assignments (`/=`), or scalar division (`/`).
    - Whitespace (` `, `\r`, `\t`): Silently discarded.
    - Structural Newlines (`\n`): Increments `line`, resets `column = 1`, emits `NEWLINE`, and invokes `handleIndentation()`.
    - String Literals (`"`, `'`): Routes to `string()`, `charLiteral()`.
    - Identifiers and Numerics: Routes characters matching `isdigit()` to `number()` and `isalpha() || '_' || >= 128` to `identifier()`.
    - Fallback: Calls `manageError("Unexpected character")` and `recoverFromError()`.
- **Dependencies & Invocations**: Invokes all primitive operations, literal parsers, and token emission helpers.

### `handleIndentation()`
- **Signature**: `void handleIndentation()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Evaluates layout geometry following a physical newline:
    1. Scans forward from `current` into a temporary offset `tempCurrent`, tallying spaces (`+1`) and tabs (`+4`).
    2. If the line contains only whitespace or begins with comment markers (`//`, `/*`), the function aborts immediately, leaving cursor state unchanged.
    3. Commits the cursor: sets `column += spaces` and `current = tempCurrent`.
    4. Compares calculated `spaces` against `currentLevel = indentStack.back()`:
        - If `spaces > currentLevel`: Pushes `spaces` onto `indentStack` and appends `TokenType::INDENT`.
        - If `spaces < currentLevel`: Repeatedly pops `indentStack` and appends `TokenType::DEDENT` for each level until matching or underflowing. If `indentStack.back() != spaces`, triggers `manageError("Inconsistent indentation level")`.
- **Dependencies & Invocations**: Invokes `manageError()` on invalid dedent alignments.

### `number()`
- **Signature**: `void number()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Parses multi-base integers and IEEE 754 floating-point literals:
    1. Inspects leading zero `0` followed by `x`/`X` (hex, base 16), `b`/`B` (bin, base 2), or `o`/`O` (oct, base 8). Adjusts `base` and prefixes accordingly.
    2. For base 10, consumes digits, checks for fractional parts (`.` not followed by `.` or identifiers), and consumes exponential scientific notation (`e`/`E` with optional `+/-`).
    3. Extracts the string slice. Uses `std::stod` for floats and `std::stoll` for integers within a `try-catch` block.
    4. Emits `INT_LITERAL`, `HEX_LITERAL`, `BIN_LITERAL`, `OCT_LITERAL`, or `FLOAT_LITERAL` with native payload via `addToken()`. On conversion failure (overflow), routes to `manageError("Numeric literal is out of range")`.
- **Dependencies & Invocations**: Invokes `advance()`, `peek()`, `peekNext()`, `addToken()`, and `manageError()`.

### `identifier()`
- **Signature**: `void identifier()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Consumes contiguous alphanumeric, underscore, or extended UTF-8 characters (`>= 128`).
    1. Checks for string prefix decorators: if text is `"r"` and followed by `"`, dispatches to `rawString()`; if `"f"`, dispatches to `formatString()`.
    2. Computes the 32-bit FNV-1a hash of the lexeme and executes a `switch` against compile-time hashed keywords.
    3. If unmapped, classifies the token as `TokenType::IDENTIFIER`. Queries `identifierIDs`: assigns `nextID++` if unseen, then retrieves the interned ID.
    4. Emits the token via `addIdentifierToken()`.
- **Dependencies & Invocations**: Invokes `hash()`, `rawString()`, `formatString()`, and `addIdentifierToken()`.

### `string()`
- **Signature**: `void string()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Parses standard double-quoted string literals:
    1. Consumes characters until finding unescaped closing `"` or hitting `\n`/EOF.
    2. If `\n` is encountered unescaped, terminates parsing and emits `manageError("Unterminated string literal (newline found)")`.
    3. Resolves escape sequences: `\n`, `\r`, `\t`, `\b`, `\f`, `\v`, `\\`, `\"`, `\'`, `\0`. Unrecognized escape characters are retained verbatim.
    4. Consumes the trailing `"` and invokes `addToken(TokenType::STRING_LITERAL, processedValue)`.
- **Dependencies & Invocations**: Invokes `advance()`, `peek()`, `manageError()`, and `addToken()`.

### `rawString()`
- **Signature**: `void rawString()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Parses raw strings (`r"..."`). Escape sequences are ignored (e.g., `\n` is read as literal `\` and `n`), except for escaped quotes (`\"`), which are consumed to prevent premature termination. Emits `TokenType::RAW_STRING_LITERAL`.
- **Dependencies & Invocations**: Invokes `advance()`, `peek()`, `peekNext()`, `manageError()`, and `addToken()`.

### `formatString()`
- **Signature**: `void formatString()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Parses formatted interpolation strings (`f"..."`). Standard escape sequences are unescaped into character equivalents, while embedded interpolation markers (`{ ... }`) are preserved in the text payload for subsequent parsing by `Parser::parseFormatString()`. Emits `TokenType::FORMAT_STRING_LITERAL`.
- **Dependencies & Invocations**: Invokes `advance()`, `peek()`, `manageError()`, and `addToken()`.

### `charLiteral()`
- **Signature**: `void charLiteral()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Parses character literals enclosed in single quotes (`'...'`):
    1. Rejects empty character literals (`''`).
    2. Resolves standard escape sequences or invokes `decodeUtf8()` for multi-byte code points.
    3. Verifies that the literal is closed by a trailing `'` delimiter. Rejects multi-character sequences.
    4. Emits `TokenType::CHAR_LITERAL` carrying the decoded 32-bit scalar value.
- **Dependencies & Invocations**: Invokes `decodeUtf8()`, `manageError()`, and `addToken()`.

### `decodeUtf8()`
- **Signature**: `uint32_t decodeUtf8()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Consumes a valid UTF-8 byte sequence and decodes it into a 32-bit Unicode code point:
    - 1-byte (`0xxxxxxx`): Directly returns the byte value.
    - 2-byte (`110xxxxx`): Consumes 1 continuation byte (`10xxxxxx`), extracting an 11-bit code point.
    - 3-byte (`1110xxxx`): Consumes 2 continuation bytes, extracting a 16-bit code point.
    - 4-byte (`11110xxx`): Consumes 3 continuation bytes, extracting a 21-bit code point.
    - Validates bit patterns; if invalid or truncated, invokes `manageError()` and returns `0`.
- **Dependencies & Invocations**: Invokes `advance()`, `isAtEnd()`, and `manageError()`.

### `manageError(const std::string&)`
- **Signature**: `void manageError(const std::string& message)`
- **Parameters & Fields**:
    - `message`: Error description detailing the scanning failure.
- **Technical Purpose & Mechanics**: Extracts the offending source slice `source.substr(start, current - start)`. Prints an error message to `std::cerr` detailing the line, starting column, error message, and offending text. Appends an error token (`TokenType::ERROR`) to `tokens` so the Parser can initiate panic recovery.
- **Dependencies & Invocations**: Pushes `TokenType::ERROR` to `tokens`.

### `recoverFromError()`
- **Signature**: `void recoverFromError()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Implements panic-mode recovery for the scanner. Discards characters until the scan head reaches whitespace or structural punctuation (`(`, `)`, `{`, `}`, `,`, `;`), re-synchronizing the scanner at a reliable boundary.
- **Dependencies & Invocations**: Consumes characters via `peek()`, `advance()`, and `isspace()`.

### `addToken(TokenType)`
- **Signature**: `void addToken(TokenType type)`
- **Parameters & Fields**:
    - `type`: Target `TokenType`.
- **Technical Purpose & Mechanics**: Extracts the substring from `start` to `current` and appends a `Token(type, text, line, startColumn)` to `tokens`.
- **Dependencies & Invocations**: Pushes to `tokens`.

### `addIdentifierToken(TokenType, int)`
- **Signature**: `void addIdentifierToken(TokenType type, int id)`
- **Parameters & Fields**:
    - `type`: `TokenType::IDENTIFIER` or keyword token type.
    - `id`: Dense interned identifier integer.
- **Technical Purpose & Mechanics**: Extracts the substring from `start` to `current` and appends a `Token(type, text, line, startColumn, id)` to `tokens`.
- **Dependencies & Invocations**: Pushes to `tokens`.

### `addToken(TokenType, LiteralValue)`
- **Signature**: `void addToken(TokenType type, LiteralValue value)`
- **Parameters & Fields**:
    - `type`: Target literal `TokenType`.
    - `value`: Parsed `LiteralValue` variant moved into the token.
- **Technical Purpose & Mechanics**: Extracts the substring from `start` to `current` and appends a `Token(type, text, line, startColumn, std::move(value))` to `tokens`.
- **Dependencies & Invocations**: Pushes to `tokens`.
---
# Abstract Syntax Tree (AST Module)

The Abstract Syntax Tree (AST) subsystem provides the strongly-typed structural intermediate representation of the WOLF programming language. It models the syntactic hierarchy of programs parsed from tokens, serving as the immutable contract ingested by the Semantic Analyzer and downstream compiler tooling.

### Closed Sum-Type Polymorphism via `std::variant`
Unlike traditional object-oriented compilers that rely on open class hierarchies with dynamic dispatch (`virtual` methods and runtime type information `dynamic_cast`), the WOLF AST utilizes **closed sum-type polymorphism**. All expression and statement variants are modeled as compile-time variants (`ExprVariant` and `StmtVariant`) wrapped in value containers (`Expr` and `Stmt`).
- **Memory Invariant**: Polymorphic nodes do not incur the memory or dereferencing overhead of virtual method tables (`vtable` pointers).
- **Visitor Ergonomics**: Node inspection and transformation are driven by `std::visit`, guaranteeing exhaustive pattern matching, static type verification, and branch inlining during compiler optimization.

### Ownership, Tree Topography, and Decoupling
- **Exclusive Tree Hierarchy**: Inner nodes and branches are owned exclusively via `std::unique_ptr<T>`. Node sharing across subtrees is prohibited, guaranteeing an acyclic tree topology.
- **Header Decoupling (`ASTdec.h` vs. `ASTNodes.h`)**: Circular type dependencies between statements (which contain expressions) and expressions (which can enclose nested statements or blocks) are decoupled through `ASTdec.h`. It forward-declares all node structs and defines the type-erased variants `Expr` and `Stmt`. Concrete struct layouts and AST factory templates are defined in `ASTNodes.h`.

```
                  ┌───────────────────────┐
                  │      ASTdec.h         │
                  │ Forward Declarations  │
                  │  ExprVariant / Expr   │
                  │  StmtVariant / Stmt   │
                  └───────────┬───────────┘
                              │ Included By
                              ▼
                  ┌───────────────────────┐
                  │      ASTNodes.h       │
                  │ Concrete Node Structs │
                  │ makeExpr / makeStmt   │
                  └───────────────────────┘
```

---

## Enum Class: `AccessModifier`

Defines member accessibility within aggregate class definitions.

- **Underlying Type**: Implicit `int`.
- **Structural Invariant**: Constrains visibility of class fields and methods.
- **Categorization**:
    - `PUBLIC`: Accessible from all scopes and external consumers.
    - `PRIVATE`: Accessible exclusively from methods declared within the enclosing class.
    - `PROTECTED`: Accessible from the enclosing class and derived subclasses.

---

## Struct: `ClassMember`

Wraps a class-level member declaration along with its associated access modifier.

- **Structural Invariant**: `declaration` contains a `Stmt` wrapping either a `VarDeclStmt` (field) or `FunctionDeclStmt` (method).
- **Memory Footprint**: `AccessModifier access` (4 bytes) + `Stmt declaration` (16 bytes) + padding = ~24 bytes.
- **Ownership Semantics**: Value container owning the wrapped member declaration.

### Member Fields
- `AccessModifier access`: Declared visibility level.
- `Stmt declaration`: Polymorphic statement node representing a field or method.

---

## Alias: `ExprVariant`

- **Definition**:
  ```cpp
  using ExprVariant = std::variant<
      std::unique_ptr<LiteralExpr>,
      std::unique_ptr<VariableExpr>,
      std::unique_ptr<BinaryExpr>,
      std::unique_ptr<RangeExpr>,
      std::unique_ptr<UnaryExpr>,
      std::unique_ptr<GroupingExpr>,
      std::unique_ptr<CallExpr>,
      std::unique_ptr<MemberAccessExpr>,
      std::unique_ptr<ArrayAccessExpr>,
      std::unique_ptr<ArrayLiteralExpr>,
      std::unique_ptr<TernaryExpr>,
      std::unique_ptr<ThisExpr>,
      std::unique_ptr<BaseExpr>,
      std::unique_ptr<CastExpr>
  >;
  ```
- **Structural Invariant**: Closed sum type encapsulating all valid expression nodes.
- **Memory Footprint**: 16 bytes (size of `std::unique_ptr` plus variant index discriminant and alignment).

---

## Struct: `Expr`

Value container wrapping an expression variant.

- **Structural Invariant**: `as` holds an active non-null `std::unique_ptr` to a concrete expression node.
- **Memory Footprint**: 16 bytes.
- **Lifecycle & Ownership**: Move-only semantics inherited from `std::unique_ptr`.

### Member Fields
- `ExprVariant as`: Active polymorphic expression variant.

---

## Alias: `StmtVariant`

- **Definition**:
  ```cpp
  using StmtVariant = std::variant<
      std::unique_ptr<ExpressionStmt>,
      std::unique_ptr<VarDeclStmt>,
      std::unique_ptr<BlockStmt>,
      std::unique_ptr<IfStmt>,
      std::unique_ptr<WhileStmt>,
      std::unique_ptr<ForStmt>,
      std::unique_ptr<FunctionDeclStmt>,
      std::unique_ptr<StructDeclStmt>,
      std::unique_ptr<ClassDeclStmt>,
      std::unique_ptr<ReturnStmt>,
      std::unique_ptr<BreakStmt>,
      std::unique_ptr<ContinueStmt>
  >;
  ```
- **Structural Invariant**: Closed sum type encapsulating all valid statement nodes.
- **Memory Footprint**: 16 bytes.

---

## Struct: `Stmt`

Value container wrapping a statement variant.

- **Structural Invariant**: `as` holds an active non-null `std::unique_ptr` to a concrete statement node.
- **Memory Footprint**: 16 bytes.
- **Lifecycle & Ownership**: Move-only semantics inherited from `std::unique_ptr`.

### Member Fields
- `StmtVariant as`: Active polymorphic statement variant.

---

## Struct: `Parameter`

Models a formal subroutine parameter definition.

- **Structural Invariant**: `type` specifies parameter type; `name` specifies identifier; `defaultValue` is optional (`nullptr` if omitted).
- **Memory Footprint**: `Token type` (~80 bytes), `Token name` (~80 bytes), `std::unique_ptr<Expr> defaultValue` (8 bytes) = ~168 bytes.

### Member Fields
- `Token type`: Type token specifying the parameter contract.
- `Token name`: Identifier token naming the parameter.
- `std::unique_ptr<Expr> defaultValue`: Optional default argument expression.

---

## Struct: `ElseIfBranch`

Represents an intermediate conditional branch (`elif`) within an `IfStmt`.

- **Structural Invariant**: `condition` evaluates to a boolean expression; `block` contains the guarded execution block.
- **Memory Footprint**: `Token keyword` (~80 bytes), `Expr condition` (16 bytes), `Stmt block` (16 bytes) = ~112 bytes.

### Member Fields
- `Token keyword`: The `elif` token anchor.
- `Expr condition`: Guard expression.
- `Stmt block`: Statement node containing the body block.

---

## Expression Nodes

### Struct: `LiteralExpr`
- **Structural Invariant**: Represents constant literal scalar values.
- **Fields**:
    - `Token value`: Token encapsulating literal type and parsed payload.

### Struct: `VariableExpr`
- **Structural Invariant**: Represents an identifier referencing a variable, parameter, or constant.
- **Fields**:
    - `Token name`: Identifier token carrying symbol name and interned ID.

### Struct: `BinaryExpr`
- **Structural Invariant**: Represents binary infix operations ($Left \odot Right$).
- **Fields**:
    - `Expr left`: Left operand subtree.
    - `Token op`: Infix operator token (`+`, `-`, `*`, `==`, `=`, etc.).
    - `Expr right`: Right operand subtree.

### Struct: `RangeExpr`
- **Structural Invariant**: Represents integer interval ranges ($start \dots end$).
- **Fields**:
    - `Expr start`: Lower boundary expression.
    - `Token op`: Range operator token (`..`).
    - `Expr end`: Upper boundary expression.

### Struct: `UnaryExpr`
- **Structural Invariant**: Models prefix or postfix unary operations.
- **Fields**:
    - `Token op`: Operator token (`!`, `-`, `+`, `~`, `++`, `--`).
    - `Expr operand`: Expression operand being modified or evaluated.
    - `bool isPostfix`: Set to `true` for postfix operations (`x++`); `false` for prefix (`++x`).

### Struct: `GroupingExpr`
- **Structural Invariant**: Represents explicit parenthetical grouping `(expr)`.
- **Fields**:
    - `Token openingParen`: Opening `(` token for diagnostic tracking.
    - `Expr expression`: Inner expression subtree.

### Struct: `ThisExpr`
- **Structural Invariant**: Represents an explicit self-instance reference within a class method.
- **Fields**:
    - `Token keyword`: The `this` token anchor.

### Struct: `BaseExpr`
- **Structural Invariant**: Represents a reference to the superclass instance for base method/field access.
- **Fields**:
    - `Token keyword`: The `base` token anchor.

### Struct: `CallExpr`
- **Structural Invariant**: Represents function, method, or constructor invocation.
- **Fields**:
    - `Expr callee`: Target expression being invoked (function pointer, variable, or member access).
    - `Token paren`: Consumed opening `(` token anchor.
    - `std::vector<Expr> arguments`: Sequential argument expressions passed to the callee.

### Struct: `MemberAccessExpr`
- **Structural Invariant**: Represents field dereference or method access on an aggregate instance (`object.member`).
- **Fields**:
    - `Expr accessed`: Base object expression.
    - `Token dot`: The `.` token anchor.
    - `Token member`: Identifier token of the targeted field or method.

### Struct: `ArrayAccessExpr`
- **Structural Invariant**: Represents index subscripting into an array (`array[index]`).
- **Fields**:
    - `Expr array`: Base array expression.
    - `Token openingBracket`: The `[` token anchor.
    - `Expr index`: Subscript index expression.

### Struct: `ArrayLiteralExpr`
- **Structural Invariant**: Represents array literal instantiation (`[e0, e1, ...]`).
- **Fields**:
    - `Token openingBracket`: The `[` token anchor.
    - `std::vector<Expr> elements`: Contained element expressions.

### Struct: `TernaryExpr`
- **Structural Invariant**: Represents conditional expressions ($Cond \ ? \ TrueBranch \ : \ FalseBranch$).
- **Fields**:
    - `Expr condition`: Boolean guard expression.
    - `Token questionMark`: The `?` token anchor.
    - `Expr trueBranch`: Expression evaluated when condition holds true.
    - `Expr falseBranch`: Expression evaluated when condition holds false.

### Struct: `CastExpr`
- **Structural Invariant**: Represents explicit type casting (`expr as TargetType`).
- **Fields**:
    - `Expr expression`: Base expression to cast.
    - `Token asToken`: The `as` token anchor.
    - `Token targetType`: Target type token.

---

## Statement Nodes

### Struct: `ExpressionStmt`
- **Structural Invariant**: Wraps an expression used as an independent statement.
- **Fields**:
    - `Expr expression`: Evaluated expression.

### Struct: `VarDeclStmt`
- **Structural Invariant**: Declares a variable with optional initial value and mutability constraint.
- **Fields**:
    - `Token type`: Declared type token.
    - `Token name`: Identifier token.
    - `std::unique_ptr<Expr> initializer`: Optional initialization expression (`nullptr` if uninitialized).
    - `bool isConst`: Set to `true` if declared with `const`.

### Struct: `BlockStmt`
- **Structural Invariant**: Represents an indentation-delimited block of statements.
- **Fields**:
    - `std::vector<Stmt> statements`: Sequential statements contained within the block scope.

### Struct: `IfStmt`
- **Structural Invariant**: Represents conditional execution branching.
- **Fields**:
    - `Token keyword`: The `if` token anchor.
    - `Expr condition`: Guard expression.
    - `Stmt thenBranch`: Statement block executed when condition holds true.
    - `std::vector<ElseIfBranch> elifBranches`: Sequential `elif` branch structures.
    - `std::unique_ptr<Stmt> elseBranch`: Optional fallback statement block (`nullptr` if omitted).

### Struct: `WhileStmt`
- **Structural Invariant**: Represents condition-controlled iteration.
- **Fields**:
    - `Token keyword`: The `while` token anchor.
    - `Expr condition`: Loop continuation condition.
    - `Stmt body`: Loop body block.

### Struct: `ForStmt`
- **Structural Invariant**: Represents range or collection iteration.
- **Fields**:
    - `Token keyword`: The `for` token anchor.
    - `Token iteratorVar`: Local iterator variable identifier token.
    - `Expr iterable`: Iterable expression (range or array).
    - `Stmt body`: Loop body block.

### Struct: `ReturnStmt`
- **Structural Invariant**: Subroutine exit statement.
- **Fields**:
    - `Token keyword`: The `return` token anchor.
    - `std::unique_ptr<Expr> value`: Optional returned expression (`nullptr` for void return).

### Struct: `BreakStmt`
- **Structural Invariant**: Terminates innermost enclosing loop.
- **Fields**:
    - `Token keyword`: The `break` token anchor.

### Struct: `ContinueStmt`
- **Structural Invariant**: Advances to the next iteration of innermost enclosing loop.
- **Fields**:
    - `Token keyword`: The `continue` token anchor.

### Struct: `StructDeclStmt`
- **Structural Invariant**: Defines a value-semantic aggregate struct layout.
- **Fields**:
    - `Token name`: Struct identifier token.
    - `std::vector<Stmt> fields`: Contained field declarations (each holding a `VarDeclStmt`).

### Struct: `ClassDeclStmt`
- **Structural Invariant**: Defines an OOP nominal class supporting inheritance and encapsulation.
- **Fields**:
    - `Token name`: Class identifier token.
    - `std::optional<Token> superclass`: Optional base class identifier token.
    - `std::vector<ClassMember> members`: Field and method declarations paired with access modifiers.

### Struct: `FunctionDeclStmt`
- **Structural Invariant**: Declares or defines a subroutine.
- **Fields**:
    - `Token returnType`: Declared return type token.
    - `Token name`: Subroutine identifier token.
    - `std::vector<Parameter> parameters`: Ordered parameter contracts.
    - `Stmt body`: Function body block statement.

---

## Standalone Factory Functions

### `makeExpr`
- **Signature**: `template<typename T, typename... Args> Expr makeExpr(Args&&... args)`
- **Parameters & Fields**:
    - `args`: Universal reference pack forwarded to construct type `T`.
- **Technical Purpose & Mechanics**: Allocates a concrete expression node `T` on the heap via `std::make_unique<T>`, packaging it into an `Expr` value container.
- **Dependencies & Invocations**: Consumed by Parser expression-building methods.

### `makeStmt`
- **Signature**: `template<typename T, typename... Args> Stmt makeStmt(Args&&... args)`
- **Parameters & Fields**:
    - `args`: Universal reference pack forwarded to construct type `T`.
- **Technical Purpose & Mechanics**: Allocates a concrete statement node `T` on the heap via `std::make_unique<T>`, packaging it into a `Stmt` value container.
- **Dependencies & Invocations**: Consumed by Parser statement-building methods.

---
# Parser - Syntactic Analysis 

The Syntactic Analysis subsystem converts the linear token stream produced by the Lexer into an Abstract Syntax Tree (AST). It enforces the formal grammar of the WOLF programming language, models operator binding semantics, and packages program logic into strongly-typed nodes (`Stmt` and `Expr`).

### Hybrid Parsing Architecture
The parser implements a dual-engine architecture:
1. **Recursive Descent Engine (Statements & Declarations)**: Top-level constructs, compound structures (`class`, `struct`), control-flow branches (`if`, `elif`, `else`, `while`, `for`), subroutine definitions, and indentation-delimited blocks are parsed via deterministic recursive descent. Grammatical ambiguity is resolved using arbitrary lookahead scanning (`isDeclaration`, `peeknNext`).
2. **Pratt Parsing Engine (Expressions & Precedence Climbing)**: Expressions are parsed using a Top-Down Operator Precedence (Pratt) algorithm. Operators are decoupled into Null Denotation (`Nud`, prefix/standalone operands) and Left Denotation (`Led`, infix/postfix operands) dispatch tables. Operator precedence levels (`Precedence`) and associativity determine subtree grouping without requiring deeply nested grammar production methods.

```
 Token Stream ──► [ Parser::parse() ]
                        │
       ┌────────────────┴────────────────┐
       ▼                                 ▼
Recursive Descent                 Pratt Expression Engine
 (Statements, Declarations)        (Nud / Led Precedence Table)
       │                                 │
       └────────────────┬────────────────┘
                        ▼
         Abstract Syntax Tree (std::vector<Stmt>)
```

### Indentation-Bound Block Parsing
Block scoping in WOLF relies on synthetic tokens emitted by the Lexer. The method `parseBlock()` asserts that nested statement blocks begin with `TokenType::INDENT` and terminate strictly at `TokenType::DEDENT`. Intermediate physical newlines (`\n`) are drained, and inner statements are parsed recursively into a `BlockStmt`.

### String Interpolation Lowering
The parser natively desugars format strings (`f"..."`). Upon encountering `TokenType::FORMAT_STRING_LITERAL`, the parser splits the literal at `{` and `}` delimiters. Interleaved expressions are tokenized and parsed in isolated sub-parsers and lowered into a balanced binary tree of string concatenations (`+`).

### Error Recovery & Panic Mode
When syntactically malformed input is detected, the parser records the error, constructs a `ParseError` exception, and unwinds to `synchronize()`. In panic mode, tokens are consumed until a synchronization boundary is encountered (a `NEWLINE` or statement introducer like `class`, `struct`, `if`, `while`, `for`, `return`, `INDENT`, `DEDENT`), allowing the parser to resume syntax verification without cascading failures.

---

## Enum Class: `Precedence`

Defines the binding power ladder used by the Pratt parser to resolve operator precedence and associativity.

- **Underlying Type**: Implicit `int`.
- **Structural Invariant**: Higher numerical values bind more tightly to operands than lower values.
- **Precedence Levels**:
    - `NONE = 0`: Lowest binding power; sentinel for tokens that cannot act as operators.
    - `ASSIGNMENT = 1`: Assignment and compound assignment operators (`=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`, `?`).
    - `OR = 2`: Logical disjunction (`||`).
    - `AND = 3`: Logical conjunction (`&&`).
    - `EQUALITY = 4`: Equality and inequality operators (`==`, `!=`).
    - `COMPARISON = 5`: Relational operators (`<`, `<=`, `>`, `>=`), range constructor (`..`), casting (`as`), and type testing (`is`).
    - `TERM = 6`: Additive and bitwise operators (`+`, `-`, `|`, `^`).
    - `FACTOR = 7`: Multiplicative, division, bitwise AND, and shift operators (`*`, `/`, `%`, `&`, `<<`, `>>`).
    - `UNARY = 8`: Prefix unary operations (`!`, `-`, `+`, `~`, prefix `++`, prefix `--`).
    - `CALL = 9`: Postfix operations, member access, subscripts, and invocations (`.`, `()`, `[]`, postfix `++`, postfix `--`).
    - `PRIMARY = 10`: Highest binding power (literals, variables, grouped expressions).

---

## Class: `ParseError`

Custom runtime exception utilized internally by the parser to trigger unwinding and transfer control to synchronization points.

- **Structural Invariant**: Inherits from `std::runtime_error`. Holds an empty error string as diagnostic messages are logged directly to the error stream before throwing.
- **Memory Footprint**: Standard library exception object footprint (~16 bytes).
- **Lifecycle & Ownership**: Value-instantiated during error handling and caught by `parse()`.

---

## Class: `Parser`

The syntax analysis engine that consumes tokens and constructs AST nodes.

- **Structural Invariant**:
    - `tokens` is an immutable, non-null reference to the token stream terminating in `TokenType::END_OF_FILE`.
    - `current <= tokens.size()`.
- **Memory Footprint**: `const std::vector<Token>& tokens` (8-byte pointer reference) + `size_t current` (8 bytes) = 16 bytes total.
- **Lifecycle & Ownership**: Does not own the token stream. Owns and returns the generated AST tree nodes via `std::unique_ptr`.

### Member Fields
- `const std::vector<Token>& tokens`: Reference to the input token stream.
- `size_t current`: Integer index of the next token to be consumed.

---

### Constructors & Primary Entry Points

### `Parser(const std::vector<Token>&)`
- **Signature**: `explicit Parser(const std::vector<Token>& tokens)`
- **Parameters & Fields**:
    - `tokens`: Constant reference to the validated token sequence produced by `Lexer`.
- **Technical Purpose & Mechanics**: Initializes the parser with its token stream and sets the cursor `current = 0`.
- **Dependencies & Invocations**: Invoked directly by pipeline orchestrators (e.g., `main.cpp`).

### `parse()`
- **Signature**: `std::vector<Stmt> parse()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Executes the top-level parsing loop:
    1. Iterates while `!isAtEnd()`.
    2. Drains extraneous top-level `NEWLINE` tokens.
    3. Invokes `parseStatement()` inside a `try-catch` block.
    4. Upon catching `ParseError`, delegates to `synchronize()` to restore the parser to a valid statement boundary.
    5. Returns the collected AST statement vector.
- **Dependencies & Invocations**: Invokes `isAtEnd()`, `match()`, `parseStatement()`, and `synchronize()`.

---

### Statement Parsers (Recursive Descent)

### `parseStatement()`
- **Signature**: `Stmt parseStatement()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Acts as the primary statement dispatcher:
    1. Checks `isDeclaration()`: if true, computes lookahead to distinguish between `parseFunctionDecl()` (type followed by identifier and `(`) and `parseVarDecl()`.
    2. Matches statement keywords: `if` (`parseIfStmt`), `while` (`parseWhileStmt`), `for` (`parseForStmt`), `struct` (`parseStructDecl`), `class` (`parseClassDecl`), `return` (`parseReturnStmt`), `break` (`parseBreakStmt`), `continue` (`parseContinueStmt`).
    3. Fallback: Parses an expression statement via `parseExpression()`, consumes a terminating `NEWLINE`, and packages it into `ExpressionStmt`.
- **Dependencies & Invocations**: Invokes `isDeclaration()`, all statement parsers, `parseExpression()`, and `consume()`.

### `parseBlock()`
- **Signature**: `BlockStmt parseBlock()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Parses an indented block:
    1. Consumes a mandatory `TokenType::INDENT`.
    2. Loops while `!check(TokenType::DEDENT)` and `!isAtEnd()`.
    3. Skips empty lines (`NEWLINE`).
    4. Parses sequential statements via `parseStatement()`.
    5. Consumes a mandatory `TokenType::DEDENT` and returns a `BlockStmt`.
- **Dependencies & Invocations**: Invokes `consume()`, `check()`, `isAtEnd()`, `match()`, and `parseStatement()`.

### `parseVarDecl()`
- **Signature**: `Stmt parseVarDecl()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Parses variable declarations (`[const] Type name [= expr]\n`):
    1. Matches optional `TokenType::KW_CONST`.
    2. Verifies and advances the base type token.
    3. Consumes arbitrary trailing array brackets (`[]`) and nullable operators (`?`), appending them to `typeVar.lexeme`.
    4. Consumes the variable `TokenType::IDENTIFIER`.
    5. Inspects optional assignment (`=`): if present, parses the initialization expression.
    6. Consumes `TokenType::NEWLINE` and returns a `VarDeclStmt`.
- **Dependencies & Invocations**: Invokes `match()`, `isTypeToken()`, `advance()`, `consume()`, `parseExpression()`, and `makeStmt<VarDeclStmt>()`.

### `parseFunctionDecl()`
- **Signature**: `Stmt parseFunctionDecl()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Parses subroutines (`ReturnType name(param1, ...) : \n block`):
    1. Advances the return type and accumulates `[]` or `?` modifiers into its lexeme.
    2. Consumes the function name identifier and opening `(`.
    3. Parses a comma-separated parameter list: extracts parameter types (with modifiers), identifiers, and optional default values (`= expr`).
    4. Consumes `)`, `:`, and a trailing `NEWLINE`.
    5. Parses the body via `parseBlock()` and returns a `FunctionDeclStmt`.
- **Dependencies & Invocations**: Invokes `advance()`, `match()`, `consume()`, `parseExpression()`, `parseBlock()`, and `makeStmt<FunctionDeclStmt>()`.

### `parseStructDecl()`
- **Signature**: `Stmt parseStructDecl()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Parses data aggregate layouts (`struct Name:\n fields...`):
    1. Consumes `TokenType::KW_STRUCT`, the struct identifier, `:`, and `NEWLINE`.
    2. Consumes `TokenType::INDENT`.
    3. Loops until `TokenType::DEDENT`, parsing field declarations via `parseVarDecl()`. Rejects non-variable declarations.
    4. Consumes `TokenType::DEDENT` and returns a `StructDeclStmt`.
- **Dependencies & Invocations**: Invokes `consume()`, `check()`, `isAtEnd()`, `match()`, `isTypeToken()`, `parseVarDecl()`, and `error()`.

### `parseClassDecl()`
- **Signature**: `Stmt parseClassDecl()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Parses OOP classes (`class Name [extends Base]:\n members...`):
    1. Consumes `TokenType::KW_CLASS` and the class identifier.
    2. Checks for inheritance: matches `TokenType::KW_EXTENDS` and consumes the superclass identifier.
    3. Consumes `:`, `NEWLINE`, and `TokenType::INDENT`.
    4. Loops until `TokenType::DEDENT`. Maintains `currentAccess` (defaults to `AccessModifier::PRIVATE`). Updates visibility when encountering `public:`, `protected:`, or `private:`.
    5. Determines member classification via lookahead: distinguishes between methods (e.g., `void foo(`, `Type? foo(`, `Type[] foo(`) and fields, invoking `parseFunctionDecl()` or `parseVarDecl()` respectively.
    6. Consumes `TokenType::DEDENT` and returns a `ClassDeclStmt`.
- **Dependencies & Invocations**: Invokes `consume()`, `match()`, `check()`, `advance()`, `peeknNext()`, `parseFunctionDecl()`, and `parseVarDecl()`.

### `parseIfStmt()`
- **Signature**: `Stmt parseIfStmt()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Parses conditional control flow:
    1. Consumes `TokenType::KW_IF`, evaluates the condition via `parseExpression()`, and consumes `:` followed by `NEWLINE`.
    2. Parses the `then` branch via `parseBlock()`.
    3. Iteratively checks for `TokenType::KW_ELIF`: consumes the keyword, parses the condition, `:`, `NEWLINE`, and body via `parseBlock()`, storing them in an `elifBranches` list.
    4. Checks for an optional `TokenType::KW_ELSE`: consumes `:`, `NEWLINE`, and parses the fallback block via `parseBlock()`.
    5. Packages nodes into `IfStmt`.
- **Dependencies & Invocations**: Invokes `consume()`, `parseExpression()`, `parseBlock()`, `check()`, `advance()`, and `match()`.

### `parseWhileStmt()`
- **Signature**: `Stmt parseWhileStmt()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Consumes `TokenType::KW_WHILE`, parses the loop condition, consumes `:` and `NEWLINE`, parses the loop body via `parseBlock()`, and constructs a `WhileStmt`.
- **Dependencies & Invocations**: Invokes `consume()`, `parseExpression()`, `parseBlock()`, and `makeStmt<WhileStmt>()`.

### `parseForStmt()`
- **Signature**: `Stmt parseForStmt()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Parses range/collection iteration (`for var in iterable:\n body`):
    1. Consumes `TokenType::KW_FOR`, the iterator variable identifier, and `TokenType::KW_IN`.
    2. Parses the iterable expression (e.g., a range `0..10` or array) via `parseExpression()`.
    3. Consumes `:`, `NEWLINE`, and parses the loop body via `parseBlock()`.
    4. Returns a `ForStmt`.
- **Dependencies & Invocations**: Invokes `consume()`, `parseExpression()`, `parseBlock()`, and `makeStmt<ForStmt>()`.

### `parseReturnStmt()`
- **Signature**: `Stmt parseReturnStmt()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Consumes `TokenType::KW_RETURN`. If the following token is not `NEWLINE`, parses the return value expression via `parseExpression()`. Consumes `NEWLINE` and returns a `ReturnStmt`.
- **Dependencies & Invocations**: Invokes `consume()`, `check()`, `parseExpression()`, and `makeStmt<ReturnStmt>()`.

### `parseBreakStmt()`
- **Signature**: `Stmt parseBreakStmt()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Consumes `TokenType::KW_BREAK`, verifies a trailing `NEWLINE`, and returns a `BreakStmt`.
- **Dependencies & Invocations**: Invokes `consume()` and `makeStmt<BreakStmt>()`.

### `parseContinueStmt()`
- **Signature**: `Stmt parseContinueStmt()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Consumes `TokenType::KW_CONTINUE`, verifies a trailing `NEWLINE`, and returns a `ContinueStmt`.
- **Dependencies & Invocations**: Invokes `consume()` and `makeStmt<ContinueStmt>()`.

---

### Expression Parsers (Pratt Precedence Engine)

### `parseExpression()`
- **Signature**: `Expr parseExpression()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Entry point for expression parsing. Initiates Pratt parsing at the lowest binding power: `parsePrecedence(Precedence::ASSIGNMENT)`.
- **Dependencies & Invocations**: Invokes `parsePrecedence()`.

### `parsePrecedence(Precedence)`
- **Signature**: `Expr parsePrecedence(Precedence precedence)`
- **Parameters & Fields**:
    - `precedence`: Current binding power threshold.
- **Technical Purpose & Mechanics**: The core Pratt parsing loop:
    1. Consumes the first token via `advance()` and retrieves its prefix rule (`NudHandlerMethod`) via `getNudHandler()`. If null, throws `ParseError`.
    2. Evaluates the prefix rule to construct the `left` expression node.
    3. Loops while the upcoming operator's precedence (`getPrecedence(peek().type)`) is greater than or equal to `precedence`.
    4. Consumes the operator token via `advance()`, retrieves its infix rule (`LedHandlerMethod`) via `getLedHandler()`, and invokes it, passing `left` and `opToken` to construct the combined expression tree.
    5. Returns the consolidated `Expr`.
- **Dependencies & Invocations**: Invokes `advance()`, `getNudHandler()`, `peek()`, `getPrecedence()`, `getLedHandler()`, and handler function pointers.

### `parseLiteral(Token)`
- **Signature**: `Expr parseLiteral(Token token)`
- **Parameters & Fields**:
    - `token`: Literal token (`INT`, `FLOAT`, `STRING`, `CHAR`, `TRUE`, `FALSE`, `NULL`).
- **Technical Purpose & Mechanics**: Wraps the token inside a `LiteralExpr`.
- **Dependencies & Invocations**: Invokes `makeExpr<LiteralExpr>()`.

### `parseFormatString(Token)`
- **Signature**: `Expr parseFormatString(Token token)`
- **Parameters & Fields**:
    - `token`: Token of type `FORMAT_STRING_LITERAL`.
- **Technical Purpose & Mechanics**: Evaluates interpolated string templates:
    1. Scans the raw string content for `{` and `}` delimiters.
    2. Extracts static text slices and converts them into `LiteralExpr` string nodes.
    3. Extracts expression substrings enclosed in `{ ... }`, instantiates an isolated `Lexer` and `Parser`, and parses the nested expression.
    4. Combines all parts into a chain of binary addition (`+`) nodes (`BinaryExpr`).
    5. Returns an empty `LiteralExpr` if the format string contains no text or expressions.
- **Dependencies & Invocations**: Instantiates `Lexer` and `Parser`, invoking `tokenize()`, `parseExpression()`, and `makeExpr<BinaryExpr>()`.

### `parseThis(Token)`
- **Signature**: `Expr parseThis(Token token)`
- **Parameters & Fields**:
    - `token`: `TokenType::KW_THIS` token.
- **Technical Purpose & Mechanics**: Constructs a `ThisExpr` representing an instance reference to the enclosing class.
- **Dependencies & Invocations**: Invokes `makeExpr<ThisExpr>()`.

### `parseVariable(Token)`
- **Signature**: `Expr parseVariable(Token token)`
- **Parameters & Fields**:
    - `token`: `TokenType::IDENTIFIER` token.
- **Technical Purpose & Mechanics**: Constructs a `VariableExpr` node referencing a symbol by name.
- **Dependencies & Invocations**: Invokes `makeExpr<VariableExpr>()`.

### `parseGrouping(Token)`
- **Signature**: `Expr parseGrouping(Token openingParen)`
- **Parameters & Fields**:
    - `openingParen`: Consumed `(` token.
- **Technical Purpose & Mechanics**: Parses a parenthesized expression `(expr)`: calls `parseExpression()`, asserts a closing `)`, and returns a `GroupingExpr`.
- **Dependencies & Invocations**: Invokes `parseExpression()`, `consume()`, and `makeExpr<GroupingExpr>()`.

### `parseArrayLiteral(Token)`
- **Signature**: `Expr parseArrayLiteral(Token openingBracket)`
- **Parameters & Fields**:
    - `openingBracket`: Consumed `[` token.
- **Technical Purpose & Mechanics**: Parses comma-separated expressions within `[` and `]`, returning an `ArrayLiteralExpr`.
- **Dependencies & Invocations**: Invokes `check()`, `match()`, `parseExpression()`, `consume()`, and `makeExpr<ArrayLiteralExpr>()`.

### `parseUnary(Token)`
- **Signature**: `Expr parseUnary(Token opToken)`
- **Parameters & Fields**:
    - `opToken`: Prefix operator token (`-`, `!`, `~`, `++`, `--`).
- **Technical Purpose & Mechanics**: Parses the operand via `parsePrecedence(Precedence::UNARY)` and constructs a prefix `UnaryExpr`.
- **Dependencies & Invocations**: Invokes `parsePrecedence()` and `makeExpr<UnaryExpr>()`.

### `parsePostfix(Expr, Token)`
- **Signature**: `Expr parsePostfix(Expr left, Token opToken)`
- **Parameters & Fields**:
    - `left`: Pre-parsed left-hand operand.
    - `opToken`: Postfix operator token (`++`, `--`).
- **Technical Purpose & Mechanics**: Constructs a postfix `UnaryExpr` with `isPostfix = true`.
- **Dependencies & Invocations**: Invokes `makeExpr<UnaryExpr>()`.

### `parseBinary(Expr, Token)`
- **Signature**: `Expr parseBinary(Expr left, Token opToken)`
- **Parameters & Fields**:
    - `left`: Pre-parsed left-hand operand.
    - `opToken`: Infix binary operator token.
- **Technical Purpose & Mechanics**: Enforces left-associativity by calculating `nextPrecedence = rulePrecedence + 1` (except for right-associative assignments, where `nextPrecedence = rulePrecedence`). Parses the right-hand operand via `parsePrecedence(nextPrecedence)` and constructs a `BinaryExpr`.
- **Dependencies & Invocations**: Invokes `getPrecedence()`, `parsePrecedence()`, and `makeExpr<BinaryExpr>()`.

### `parseRange(Expr, Token)`
- **Signature**: `Expr parseRange(Expr left, Token opToken)`
- **Parameters & Fields**:
    - `left`: Left boundary expression.
    - `opToken`: `TokenType::RANGE_OP` (`..`).
- **Technical Purpose & Mechanics**: Parses the right boundary via `parsePrecedence(Precedence::COMPARISON)` and returns a `RangeExpr`.
- **Dependencies & Invocations**: Invokes `parsePrecedence()` and `makeExpr<RangeExpr>()`.

### `parseTernary(Expr, Token)`
- **Signature**: `Expr parseTernary(Expr condition, Token questionMark)`
- **Parameters & Fields**:
    - `condition`: Pre-parsed condition expression.
    - `questionMark`: Consumed `?` token.
- **Technical Purpose & Mechanics**: Parses ternary conditionals (`cond ? trueBranch : falseBranch`):
    1. Parses the true branch via `parseExpression()`.
    2. Consumes `:`.
    3. Parses the false branch using `Precedence::ASSIGNMENT` to support right-associative ternary chaining.
    4. Returns a `TernaryExpr`.
- **Dependencies & Invocations**: Invokes `parseExpression()`, `consume()`, `parsePrecedence()`, and `makeExpr<TernaryExpr>()`.

### `parseCast(Expr, Token)`
- **Signature**: `Expr parseCast(Expr left, Token asToken)`
- **Parameters & Fields**:
    - `left`: Target expression to cast.
    - `asToken`: Consumed `as` token.
- **Technical Purpose & Mechanics**: Consumes the target type token, handles array (`[]`) or nullable (`?`) suffixes, and returns a `CastExpr`.
- **Dependencies & Invocations**: Invokes `advance()`, `match()`, `consume()`, and `makeExpr<CastExpr>()`.

### `parseBase(Token)`
- **Signature**: `Expr parseBase(Token token)`
- **Parameters & Fields**:
    - `token`: `TokenType::KW_BASE` token.
- **Technical Purpose & Mechanics**: Constructs a `BaseExpr` referencing superclass methods or fields.
- **Dependencies & Invocations**: Invokes `makeExpr<BaseExpr>()`.

### `parseCall(Expr, Token)`
- **Signature**: `Expr parseCall(Expr callee, Token openingParen)`
- **Parameters & Fields**:
    - `callee`: Expression being invoked.
    - `openingParen`: Consumed `(` token.
- **Technical Purpose & Mechanics**: Parses comma-separated arguments until matching `)`, returning a `CallExpr`.
- **Dependencies & Invocations**: Invokes `check()`, `match()`, `parseExpression()`, `consume()`, and `makeExpr<CallExpr>()`.

### `parseArrayAccess(Expr, Token)`
- **Signature**: `Expr parseArrayAccess(Expr indexed, Token openingBracket)`
- **Parameters & Fields**:
    - `indexed`: Array expression being indexed.
    - `openingBracket`: Consumed `[` token.
- **Technical Purpose & Mechanics**: Parses an index expression, consumes `]`, and returns an `ArrayAccessExpr`.
- **Dependencies & Invocations**: Invokes `parseExpression()`, `consume()`, and `makeExpr<ArrayAccessExpr>()`.

### `parseMemberAccess(Expr, Token)`
- **Signature**: `Expr parseMemberAccess(Expr accessed, Token dot)`
- **Parameters & Fields**:
    - `accessed`: Target object expression.
    - `dot`: Consumed `.` token.
- **Technical Purpose & Mechanics**: Consumes an identifier for property/method access and constructs a `MemberAccessExpr`.
- **Dependencies & Invocations**: Invokes `consume()` and `makeExpr<MemberAccessExpr>()`.

---

### Pratt Lookup Tables & Helpers

### `getPrecedence(TokenType) const`
- **Signature**: `Precedence getPrecedence(TokenType type) const`
- **Parameters & Fields**:
    - `type`: Target operator token type.
- **Technical Purpose & Mechanics**: Maps token types to their corresponding `Precedence` enum level via a `switch` statement. Defaults to `Precedence::NONE`.
- **Dependencies & Invocations**: Called by `parsePrecedence()` and `parseBinary()`.

### `getNudHandler(TokenType) const`
- **Signature**: `NudHandlerMethod getNudHandler(TokenType type) const`
- **Parameters & Fields**:
    - `type`: Target prefix token type.
- **Technical Purpose & Mechanics**: Resolves the prefix handler function pointer for a token. Returns `nullptr` if the token cannot initiate an expression.
- **Dependencies & Invocations**: Called by `parsePrecedence()`.

### Alias: `NudHandlerMethod`
- **Signature**: `using NudHandlerMethod = Expr (Parser::*)(Token token);`
- **Parameters & Fields**:
    - Member function pointer taking a consumed `Token` and returning an `Expr`.
- **Technical Purpose & Mechanics**: Defines the signature for Null Denotation (`Nud`) rules. Handlers matching this signature parse tokens that require no left-hand expression (literals, prefix unary operators, grouping parentheses, array literals).
- **Dependencies & Invocations**: Bound by `Parser::getNudHandler()`, invoked by `Parser::parsePrecedence()`.

### `getLedHandler(TokenType) const`
- **Signature**: `LedHandlerMethod getLedHandler(TokenType type) const`
- **Parameters & Fields**:
    - `type`: Target infix/postfix operator token type.
- **Technical Purpose & Mechanics**: Resolves the infix/postfix handler function pointer for a token. Returns `nullptr` if the token cannot bind to a left-hand operand.
- **Dependencies & Invocations**: Called by `parsePrecedence()`.

### Alias: `LedHandlerMethod`
- **Signature**: `using LedHandlerMethod = Expr (Parser::*)(Expr left, Token opToken);`
- **Parameters & Fields**:
    - Member function pointer taking a pre-parsed `Expr left` and an operator `Token opToken`, returning an assembled `Expr`.
- **Technical Purpose & Mechanics**: Defines the signature for Left Denotation (`Led`) rules. Handlers matching this signature consume an operator that binds to an existing left-hand expression (binary arithmetic, postfix increment, function call parentheses, member access dot, array subscript bracket, ternary question mark).
- **Dependencies & Invocations**: Bound by `Parser::getLedHandler()`, invoked by `Parser::parsePrecedence()`.

---
### Navigation Helpers & Lookahead

### `advance()`
- **Signature**: `Token advance()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Increments `current` if not at EOF and returns the consumed token.
- **Dependencies & Invocations**: Invoked across all parsing routines.

### `peek()`
- **Signature**: `Token peek() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns the token at index `current` without advancing the cursor.
- **Dependencies & Invocations**: Consumed by lookahead helpers.

### `peeknNext(int) const`
- **Signature**: `Token peeknNext(int n = 1) const`
- **Parameters & Fields**:
    - `n`: Offset relative to `current` (supports negative offsets).
- **Technical Purpose & Mechanics**: Safely returns `tokens[current + n]`, clamping to `tokens.front()` or `tokens.back()` when out of bounds.
- **Dependencies & Invocations**: Used for multi-token lookahead in declaration and class member parsing.

### `consume(TokenType, const std::string&)`
- **Signature**: `Token consume(TokenType type, const std::string& message)`
- **Parameters & Fields**:
    - `type`: Expected token type.
    - `message`: Diagnostic message to emit if the token is missing.
- **Technical Purpose & Mechanics**: Advances and returns the token if it matches `type`; otherwise, logs an error via `error()` and throws `ParseError`.
- **Dependencies & Invocations**: Invokes `check()`, `advance()`, and `error()`.

### `match(TokenType)`
- **Signature**: `bool match(TokenType type)`
- **Parameters & Fields**:
    - `type`: Token type to match.
- **Technical Purpose & Mechanics**: Advances and returns `true` if `check(type)` succeeds; otherwise returns `false`.
- **Dependencies & Invocations**: Invokes `check()` and `advance()`.

### `check(TokenType) const`
- **Signature**: `bool check(TokenType type) const`
- **Parameters & Fields**:
    - `type`: Token type to compare against.
- **Technical Purpose & Mechanics**: Returns `true` if `!isAtEnd()` and `peek().type == type`.
- **Dependencies & Invocations**: Invokes `isAtEnd()` and `peek()`.

### `isAtEnd()`
- **Signature**: `bool isAtEnd() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns `true` if the current token is `TokenType::END_OF_FILE`.
- **Dependencies & Invocations**: Invoked by token inspection and parsing loops.

### `isTypeToken(TokenType) const`
- **Signature**: `bool isTypeToken(TokenType type) const`
- **Parameters & Fields**:
    - `type`: Token type to evaluate.
- **Technical Purpose & Mechanics**: Returns `true` if `type` is a primitive type keyword (`int`, `float`, `char`, `string`, `bool`, `void`) or an `IDENTIFIER` (representing a user-defined struct/class).
- **Dependencies & Invocations**: Consumed by declaration disambiguation logic.

### `isDeclaration()`
- **Signature**: `bool isDeclaration() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Performs arbitrary lookahead to determine if upcoming tokens form a variable or function declaration:
    1. Optionally skips `const`.
    2. Verifies the base type via `isTypeToken()`.
    3. Skips arbitrary bracket pairs (`[]`) and nullable markers (`?`).
    4. Checks if the subsequent token is an `IDENTIFIER`. Returns `true` if so, `false` otherwise.
- **Dependencies & Invocations**: Invokes `peeknNext()` and `isTypeToken()`.

---

### Error Handling & Recovery

### `error(const Token&, const std::string&)`
- **Signature**: `void error(const Token& token, const std::string& message)`
- **Parameters & Fields**:
    - `token`: Offending token anchor.
    - `message`: Diagnostic error message.
- **Technical Purpose & Mechanics**: Formats and prints an error message to `std::cerr` detailing the line, column, error text, and offending lexeme.
- **Dependencies & Invocations**: Called by `consume()` and parsing validation routines.

### `synchronize()`
- **Signature**: `void synchronize()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Discards tokens until reaching a synchronization boundary (`NEWLINE` or structural keywords such as `class`, `struct`, `if`, `while`, `for`, `return`, `INDENT`, `DEDENT`), preventing cascading parse failures.
- **Dependencies & Invocations**: Invokes `advance()`, `isAtEnd()`, and `peek()`.

---
# ASTPrinter - AST Visualization & Debug Tooling 

The `ASTPrinter` subsystem provides an ASCII/Unicode visualization of the Abstract Syntax Tree. It is an optional debugging tool designed to verify that the Parser's structuring, operator binding, node encapsulation, and statement nesting match the intended grammar.

### Traversal Architecture & Branch Layout
`ASTPrinter` implements the Visitor pattern over closed AST sum-types (`ExprVariant` and `StmtVariant`) via `std::visit`. It traverses the tree recursively, tracking visual indentation prefixes (`currentPrefix`) and drawing connections using Unicode box-drawing glyphs (`├──`, `└──`, `│   `).

```
--- WOLF ABSTRACT SYNTAX TREE ---
└── Statement: FunctionDeclStmt(ReturnType: int, Name: calculate) 
    ├── Parameters
    │   └── int x
    └── Body: BlockStmt
        └── Statement: ReturnStmt
            └── Value: BinaryExpr(Op: +)
                ├── Left: VariableExpr(x)
                └── Right: LiteralExpr(42)
```

---

## Class: `ASTPrinter`

Visitor class that traverses AST nodes and writes an indented tree representation to an output stream.

- **Structural Invariant**: Traversal operates read-only on the AST nodes. `currentPrefix` accurately reflects current nesting depth during traversal.
- **Memory Footprint**: `std::ostream& out` (8 bytes) + `std::string currentPrefix` (24–32 bytes) = ~32–40 bytes.
- **Lifecycle & Ownership**: Holds a non-owning reference to `std::ostream`. Does not own the AST nodes it visits.

### Member Fields
- `std::ostream& out`: Target output stream for tree rendering (defaults to `std::cout`).
- `std::string currentPrefix`: Dynamic indentation string containing branch lines.

---

### Public Interface & Branch Rendering

### `ASTPrinter(std::ostream&)`
- **Signature**: `explicit ASTPrinter(std::ostream& outputStream = std::cout)`
- **Parameters & Fields**:
    - `outputStream`: Target output stream.
- **Technical Purpose & Mechanics**: Binds the printer to the destination stream and initializes `currentPrefix` to `""`.
- **Dependencies & Invocations**: Instantiated in pipeline runners or debug harnesses.

### `printAST(const std::vector<Stmt>&)`
- **Signature**: `void printAST(const std::vector<Stmt>& statements)`
- **Parameters & Fields**:
    - `statements`: Vector of top-level AST statements.
- **Technical Purpose & Mechanics**: Prints the header `--- WOLF ABSTRACT SYNTAX TREE ---\n` and iterates through top-level statements, invoking `printBranch` for each node.
- **Dependencies & Invocations**: Invokes `printBranch()`.

### `printBranch(const Type&, bool, const std::string&)`
- **Signature**: `template<typename Type> void printBranch(const Type& branchTypeVariant, bool isLastBranch, const std::string& labelBranch = "")`
- **Parameters & Fields**:
    - `branchTypeVariant`: Node variant (`ExprVariant` or `StmtVariant`).
    - `isLastBranch`: Flag indicating whether this node is the last child of its parent.
    - `labelBranch`: Optional edge label (e.g., `"Left"`, `"Right"`, `"Body"`).
- **Technical Purpose & Mechanics**: Formats the current branch line using `└── ` (if `isLastBranch` is true) or `├── ` (otherwise), prints the optional edge label, updates `currentPrefix`, and dispatches visitor traversal via `std::visit(*this, branchTypeVariant)`. Restores the previous prefix after traversal.
- **Dependencies & Invocations**: Calls `std::visit()`.
---

### Expression Visitors

### Expression Visitors

### `operator()(const T&)`
- **Signature**: `template<typename T> const Type* operator()(const T& node)`
- **Parameters & Fields**:
    - `node`: Unhandled node variant type.
- **Technical Purpose & Mechanics**: Fallback visitor overload invoked by `std::visit` when an expression variant lacks an explicit evaluator (such as `BaseExpr`). Returns `Type::getError()` to halt type propagation safely without throwing unhandled exceptions.
- **Dependencies & Invocations**: Invoked via `std::visit(*this, expr.as)` inside `SemanticAnalyzer::evaluate()`.

### `operator()(const std::unique_ptr<LiteralExpr>&)`
- **Signature**: `void operator()(const std::unique_ptr<LiteralExpr>& node)`
- **Parameters & Fields**: `node`: Pointer to `LiteralExpr`.
- **Technical Purpose & Mechanics**: Outputs `LiteralExpr(<lexeme>)`.

### `operator()(const std::unique_ptr<VariableExpr>&)`
- **Signature**: `void operator()(const std::unique_ptr<VariableExpr>& node)`
- **Parameters & Fields**: `node`: Pointer to `VariableExpr`.
- **Technical Purpose & Mechanics**: Outputs `VariableExpr(<name>)`.

### `operator()(const std::unique_ptr<BinaryExpr>&)`
- **Signature**: `void operator()(const std::unique_ptr<BinaryExpr>& node)`
- **Parameters & Fields**: `node`: Pointer to `BinaryExpr`.
- **Technical Purpose & Mechanics**: Outputs `BinaryExpr(Op: <op>)` and recursively visits the left child (as an intermediate branch) and right child (as a terminal branch).

### `operator()(const std::unique_ptr<UnaryExpr>&)`
- **Signature**: `void operator()(const std::unique_ptr<UnaryExpr>& node)`
- **Parameters & Fields**: `node`: Pointer to `UnaryExpr`.
- **Technical Purpose & Mechanics**: Outputs `PrefixUnaryExpr` or `PostfixUnaryExpr` and prints the operand branch.

### `operator()(const std::unique_ptr<GroupingExpr>&)`
- **Signature**: `void operator()(const std::unique_ptr<GroupingExpr>& node)`
- **Parameters & Fields**: `node`: Pointer to `GroupingExpr`.
- **Technical Purpose & Mechanics**: Outputs `GroupingExpr` and prints its inner expression branch.

### `operator()(const std::unique_ptr<CallExpr>&)`
- **Signature**: `void operator()(const std::unique_ptr<CallExpr>& node)`
- **Parameters & Fields**: `node`: Pointer to `CallExpr`.
- **Technical Purpose & Mechanics**: Outputs `CallExpr`, prints the callee branch, and iterates through all argument expressions with labeled indices (`Arg[i]`).

### `operator()(const std::unique_ptr<MemberAccessExpr>&)`
- **Signature**: `void operator()(const std::unique_ptr<MemberAccessExpr>& node)`
- **Parameters & Fields**: `node`: Pointer to `MemberAccessExpr`.
- **Technical Purpose & Mechanics**: Outputs `MemberAccessExpr(Field: <member>)` and prints the target object branch.

### `operator()(const std::unique_ptr<ArrayAccessExpr>&)`
- **Signature**: `void operator()(const std::unique_ptr<ArrayAccessExpr>& node)`
- **Parameters & Fields**: `node`: Pointer to `ArrayAccessExpr`.
- **Technical Purpose & Mechanics**: Outputs `ArrayAccessExpr` and prints the array and index sub-branches.

### `operator()(const std::unique_ptr<ArrayLiteralExpr>&)`
- **Signature**: `void operator()(const std::unique_ptr<ArrayLiteralExpr>& node)`
- **Parameters & Fields**: `node`: Pointer to `ArrayLiteralExpr`.
- **Technical Purpose & Mechanics**: Outputs `ArrayLiteralExpr` and prints each element labeled as `Element[i]` (or `(Empty Array)` if empty).

### `operator()(const std::unique_ptr<TernaryExpr>&)`
- **Signature**: `void operator()(const std::unique_ptr<TernaryExpr>& node)`
- **Parameters & Fields**: `node`: Pointer to `TernaryExpr`.
- **Technical Purpose & Mechanics**: Outputs `TernaryExpr` and prints branches for `Condition`, `TrueBranch`, and `FalseBranch`.

### `operator()(const std::unique_ptr<RangeExpr>&)`
- **Signature**: `void operator()(const std::unique_ptr<RangeExpr>& node)`
- **Parameters & Fields**: `node`: Pointer to `RangeExpr`.
- **Technical Purpose & Mechanics**: Outputs `RangeExpr` and prints branches for `Start` and `End`.

### `operator()(const std::unique_ptr<CastExpr>&)`
- **Signature**: `void operator()(const std::unique_ptr<CastExpr>& node)`
- **Parameters & Fields**: `node`: Pointer to `CastExpr`.
- **Technical Purpose & Mechanics**: Outputs `CastExpr(TargetType: <targetType>)` and prints the nested expression branch.

### `operator()(const std::unique_ptr<BaseExpr>&)`
- **Signature**: `void operator()(const std::unique_ptr<BaseExpr>& node)`
- **Parameters & Fields**: `node`: Pointer to `BaseExpr`.
- **Technical Purpose & Mechanics**: Outputs `BaseExpr\n`.

### `operator()(const std::unique_ptr<ThisExpr>&)`
- **Signature**: `void operator()(const std::unique_ptr<ThisExpr>& node)`
- **Parameters & Fields**: `node`: Pointer to `ThisExpr`.
- **Technical Purpose & Mechanics**: Outputs `ThisExpr\n`.

---

### Statement Visitors

### `operator()(const std::unique_ptr<ExpressionStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<ExpressionStmt>& node)`
- **Parameters & Fields**: `node`: Pointer to `ExpressionStmt`.
- **Technical Purpose & Mechanics**: Outputs `ExpressionStmt` and prints its inner expression branch.

### `operator()(const std::unique_ptr<VarDeclStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<VarDeclStmt>& node)`
- **Parameters & Fields**: `node`: Pointer to `VarDeclStmt`.
- **Technical Purpose & Mechanics**: Outputs `VarDeclStmt(Type: <type>, Name: <name>)` and prints the optional initializer branch if present.

### `operator()(const std::unique_ptr<BlockStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<BlockStmt>& node)`
- **Parameters & Fields**: `node`: Pointer to `BlockStmt`.
- **Technical Purpose & Mechanics**: Outputs `BlockStmt` and prints each contained statement branch sequentially.

### `operator()(const std::unique_ptr<IfStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<IfStmt>& node)`
- **Parameters & Fields**: `node`: Pointer to `IfStmt`.
- **Technical Purpose & Mechanics**: Outputs `IfStmt`, prints condition and then-branch sub-branches, formats each `elif` branch with custom prefixes, and appends the optional `else` branch.

### `operator()(const std::unique_ptr<WhileStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<WhileStmt>& node)`
- **Parameters & Fields**: `node`: Pointer to `WhileStmt`.
- **Technical Purpose & Mechanics**: Outputs `WhileStmt` and prints branches for `Condition` and `Body`.

### `operator()(const std::unique_ptr<ForStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<ForStmt>& node)`
- **Parameters & Fields**: `node`: Pointer to `ForStmt`.
- **Technical Purpose & Mechanics**: Outputs `ForStmt(Iterator: <iteratorVar>)` and prints branches for `iterable` and `Body`.

### `operator()(const std::unique_ptr<ReturnStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<ReturnStmt>& node)`
- **Parameters & Fields**: `node`: Pointer to `ReturnStmt`.
- **Technical Purpose & Mechanics**: Outputs `ReturnStmt` and prints the return value branch (or `(Void Return)` if empty).

### `operator()(const std::unique_ptr<BreakStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<BreakStmt>& node)`
- **Parameters & Fields**: `node`: Pointer to `BreakStmt`.
- **Technical Purpose & Mechanics**: Outputs `BreakStmt`.

### `operator()(const std::unique_ptr<ContinueStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<ContinueStmt>& node)`
- **Parameters & Fields**: `node`: Pointer to `ContinueStmt`.
- **Technical Purpose & Mechanics**: Outputs `ContinueStmt`.

### `operator()(const std::unique_ptr<FunctionDeclStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<FunctionDeclStmt>& node)`
- **Parameters & Fields**: `node`: Pointer to `FunctionDeclStmt`.
- **Technical Purpose & Mechanics**: Outputs `FunctionDeclStmt(ReturnType: <returnType>, Name: <name>)`, formats parameter lists and default expressions, and prints the function body block.

### `operator()(const std::unique_ptr<StructDeclStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<StructDeclStmt>& node)`
- **Parameters & Fields**: `node`: Pointer to `StructDeclStmt`.
- **Technical Purpose & Mechanics**: Outputs `StructDeclStmt(Name: <name>)` and prints each field branch labeled as `Field[i]` (or `(Empty)` if no fields exist).

### `operator()(const std::unique_ptr<ClassDeclStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<ClassDeclStmt>& node)`
- **Parameters & Fields**: `node`: Pointer to `ClassDeclStmt`.
- **Technical Purpose & Mechanics**: Outputs `ClassDeclStmt(Name: <name> [, Extends: <superclass>])` and prints each member branch labeled with its access level (`Member[public]`, `Member[protected]`, `Member[private]`).
---
# Semantic Analyzer - Semantic Analysis & Type System

The Semantic Analysis subsystem enforces the static semantics of the WOLF programming language. Operating on the Abstract Syntax Tree (AST) generated by the Parser, it verifies nominal subtyping, type consistency, lexical scope boundaries, definite assignment invariants, null safety contracts, and control-flow termination guarantees.

### Two-Pass Execution Model
To support forward references and mutual recursion across user-defined aggregates without requiring forward declaration headers, the analyzer operates in two passes:
1. **Pass 1: Declaration & Layout Registration (`collectDeclarations`)**:
    - Traverses top-level statements to construct and register the nominal identities of user-defined aggregate types (`struct` and `class`) into the global symbol table.
    - Populates member variable layouts, method signatures, parameter types, and access modifiers (`public`, `protected`, `private`).
    - Resolves inheritance hierarchies (`extends`) and detects cyclic inheritance dependencies via graph cycle detection (`E0035`).
    - Registers global function prototypes to allow out-of-order function invocations.
2. **Pass 2: Statement Verification & Expression Type Inference (`typeCheck`)**:
    - Recursively traverses all statement blocks and expression subtrees using `std::visit`.
    - Performs bottom-up expression type synthesis and assignment compatibility checks.
    - Enforces definite assignment by tracking variable initialization states (`isInitialized`) and detects illegal modifications to immutable bindings (`const`).
    - Computes return path coverage (`hasGuaranteedReturn`), ensuring non-void subroutines exit via explicit return statements across all control-flow paths.
    - Performs unused variable and parameter analysis across scopes, emitting non-breaking warnings (`W0001`, `W0002`).

```
          Abstract Syntax Tree (AST)
                      │
                      ▼
 ┌──────────────────────────────────────────┐
 │ Pass 1: collectDeclarations()            │ ◄── Registers nominal types, resolves
 └────────────────────┬─────────────────────┘     inheritance cycles & member layouts
                      │ Populated SymbolTable
                      ▼
 ┌──────────────────────────────────────────┐
 │ Pass 2: typeCheck()                      │ ◄── Validates expressions, definite assignment,
 └────────────────────┬─────────────────────┘     smart-casts, return paths & access modifiers
                      │
                      ▼
      Validated AST / Diagnostic Stream
```

### Type Representation & Memory Mechanics
Types in WOLF are immutable, flyweight entities represented by the `Type` struct. Primitives (`int`, `float`, `char`, `string`, `bool`, `void`, `null`, `<error>`) are provided via static singletons. Complex types (arrays, functions, nullable wrappers) are interned in global cache tables (`arrayTypeCache`, `nullableTypeCache`) and owned by persistent pools (`typePool`, `customTypes`). This ensures canonical pointer uniqueness: type comparisons and assignment fast-paths can be evaluated using pointer identity (`this == target`).

### Null Safety & Flow-Sensitive Smart-Casting
WOLF enforces non-nullability by default. Types appended with `?` are nullable. Direct member access (`.`) or array indexing (`[]`) on nullable receivers is rejected compile-time (`E0014`, `E0015`). Within conditional statements (`if`, `elif`), the analyzer inspects binary assertions (`var != null`). In the body of a guarded branch, the analyzer introduces a localized shadowed symbol with the non-nullable base type into an isolated scope. When the block exits, the promoted symbol is popped, restoring the original nullable type signature.

---

## Enum Class: `TypeKind`

Defines the taxonomic classification of data types in the WOLF type system.

- **Underlying Type**: Implicit `int`.
- **Structural Invariant**: Categorizes every `Type` instance into a distinct semantic group.
- **Categorization**:
    - `PRIMITIVE`: Core scalar types (`int`, `float`, `char`, `string`, `bool`).
    - `ARRAY`: Homogeneous indexed collections (`T[]`).
    - `FUNCTION`: Subroutine signatures with parameter types and a return type.
    - `STRUCT`: Stack-allocated aggregate value types with public members.
    - `CLASS`: Reference types supporting inheritance and access control.
    - `VOID_TYPE`: Unit/non-value return type for procedures.
    - `NULL_TYPE`: Type of the literal `null`, assignable to any nullable type (`T?`).
    - `ERROR_TYPE`: Sentinel type used to suppress cascading error diagnostics.

---

## Struct: `MemberInfo`

Encapsulates metadata for fields and methods declared within classes and structs.

- **Structural Invariant**: `type` points to a valid, canonical `Type` instance.
- **Memory Footprint**: `const Type* type` (8 bytes), `AccessModifier access` (4 bytes), `bool isStatic` (1 byte), `bool isAbstract` (1 byte), plus padding = 16 bytes.
- **Ownership Semantics**: Non-owning reference to `Type`.

### Member Fields
- `const Type* type`: Type signature of the field or method.
- `AccessModifier access`: Visibility constraint (`PUBLIC`, `PROTECTED`, `PRIVATE`).
- `bool isStatic`: Indicates whether the member is bound to the type or to instances.
- `bool isAbstract`: Indicates whether the method lacks a concrete implementation.

---

## Struct: `Type`

The core type representation structure in the WOLF compiler.

- **Structural Invariant**: Canonical flyweight type. Primitive types exist as immutable global singletons; derived types are owned by memory pools and cached.
- **Memory Footprint**: Approximately 160–180 bytes depending on standard library container implementations.
- **Lifecycle & Ownership**: Instances cannot be copied or moved directly; allocated on heaps managed by `typePool` or `customTypes`.

### Member Fields
- `TypeKind kind`: Primary classification tag.
- `std::string name`: String representation of the type (e.g., `"int"`, `"Dog[]?"`).
- `bool isNullable`: Indicates whether the type accepts `null`.
- `const Type* elementType`: Target element type for array instances (`kind == TypeKind::ARRAY`).
- `const Type* returnType`: Return type for function instances (`kind == TypeKind::FUNCTION`).
- `std::vector<const Type*> paramTypes`: Parameter signature for function instances.
- `const Type* superclass`: Base class reference for OOP inheritance trees.
- `std::unordered_map<std::string, MemberInfo> fields`: Field member layout.
- `std::unordered_map<std::string, MemberInfo> methods`: Method member layout.

---

### Type Query Methods

### `isInteger()`
- **Signature**: `bool isInteger() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns `true` if `kind == TypeKind::PRIMITIVE && name == "int"`.
- **Dependencies & Invocations**: Consumed by arithmetic and range validation logic.

### `isFloat()`
- **Signature**: `bool isFloat() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns `true` if `kind == TypeKind::PRIMITIVE && name == "float"`.
- **Dependencies & Invocations**: Consumed by arithmetic and widening coercion checks.

### `isChar()`
- **Signature**: `bool isChar() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns `true` if `kind == TypeKind::PRIMITIVE && name == "char"`.
- **Dependencies & Invocations**: Consumed by character assignment validations.

### `isString()`
- **Signature**: `bool isString() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns `true` if `kind == TypeKind::PRIMITIVE && name == "string"`.
- **Dependencies & Invocations**: Consumed by string concatenation (`+`) and format string lowerings.

### `isBool()`
- **Signature**: `bool isBool() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns `true` if `kind == TypeKind::PRIMITIVE && name == "bool"`.
- **Dependencies & Invocations**: Consumed by conditional expressions (`if`, `while`, `? :`).

### `isVoid()`
- **Signature**: `bool isVoid() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns `true` if `kind == TypeKind::VOID_TYPE`.
- **Dependencies & Invocations**: Consumed to reject `void` variable declarations (`E0041`).

### `isError()`
- **Signature**: `bool isError() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns `true` if `kind == TypeKind::ERROR_TYPE`.
- **Dependencies & Invocations**: Consumed to short-circuit operations and prevent cascading diagnostics.

### `isNumeric()`
- **Signature**: `bool isNumeric() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns `isInteger() || isFloat()`.
- **Dependencies & Invocations**: Consumed by relational, arithmetic, and unary operator checks.

### `isNull()`
- **Signature**: `bool isNull() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns `kind == TypeKind::NULL_TYPE`.
- **Dependencies & Invocations**: Consumed by null assignment validations.

---

### Type Subtyping & Factory Methods

### `isAssignableTo(const Type*) const`
- **Signature**: `bool isAssignableTo(const Type* target) const`
- **Parameters & Fields**:
    - `target`: Destination `Type` pointer.
- **Technical Purpose & Mechanics**: Evaluates whether an expression of type `this` can be implicitly assigned or passed to a variable of type `target`:
    1. Pointer Identity: If `this == target`, returns `true`.
    2. Error Recovery: If `this->isError() || target->isError()`, returns `true` to avoid cascading errors.
    3. Void Invariance: If `this->isVoid() || target->isVoid()`, returns `false`.
    4. Null Assignability: If `this->isNull()`, returns `target->isNullable`.
    5. Nullability Invariance: If `this->isNullable && !target->isNullable`, returns `false`.
    6. Non-Null to Nullable Promotion: If `!this->isNullable && target->isNullable`, checks if `this->name + "?" == target->name`.
    7. Numeric Widening: If `this->isInteger()` and `target->isFloat()` (or `"float?"`), returns `true`.
    8. Recursive Array Subtyping: If both are arrays, evaluates `this->elementType->isAssignableTo(target->elementType)`.
    9. OOP Inheritance Hierarchy: If both are classes, traverses `this->superclass` upward to match `target` or `target->name`.
    10. Nominal Name Match: Returns `this->name == target->name`.
- **Dependencies & Invocations**: Recursive calls to `isAssignableTo()`.

### `toString()`
- **Signature**: `std::string toString() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns `this->name`.
- **Dependencies & Invocations**: Used across diagnostic messages.

### `getInt()`, `getFloat()`, `getChar()`, `getString()`, `getBool()`, `getVoid()`, `getNull()`, `getError()`
- **Signature**: `static const Type* get<Primitive>()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Return pointers to immutable static instances of built-in primitive types.
- **Dependencies & Invocations**: Consumed across all stages of semantic analysis.

### `makeFunction(const Type*, std::vector<const Type*>)`
- **Signature**: `static const Type* makeFunction(const Type* returnType, std::vector<const Type*> params)`
- **Parameters & Fields**:
    - `returnType`: Target return type.
    - `params`: Vector of parameter types.
- **Technical Purpose & Mechanics**: Constructs a function signature type `(P1, P2) -> R`, assigns its name, stores it in `typePool`, and returns a canonical non-owning pointer.
- **Dependencies & Invocations**: Pushes newly allocated `Type` to `typePool`.

### `makeArray(const Type*)`
- **Signature**: `static const Type* makeArray(const Type* element)`
- **Parameters & Fields**:
    - `element`: Element type of the array.
- **Technical Purpose & Mechanics**: Queries `arrayTypeCache`. If found, returns the cached pointer. Otherwise, creates a new `Type` with `kind = TypeKind::ARRAY`, registers it in `arrayTypeCache` and `typePool`, and returns the canonical pointer.
- **Dependencies & Invocations**: Updates `arrayTypeCache` and `typePool`.

### `makeNullable(const Type*)`
- **Signature**: `static const Type* makeNullable(const Type* base)`
- **Parameters & Fields**:
    - `base`: Non-nullable base type to wrap.
- **Technical Purpose & Mechanics**: Guards against redundant wrapping if `base` is null, nullable, void, or an error. Queries `nullableTypeCache`. If absent, clones `base`, sets `isNullable = true`, appends `?` to `name`, registers it in `nullableTypeCache` and `typePool`, and returns the pointer.
- **Dependencies & Invocations**: Updates `nullableTypeCache` and `typePool`.

---

## Enum Class: `SymbolKind`

Defines the entity classification of a symbol stored in lexical scopes.

- **Underlying Type**: Implicit `int`.
- **Categorization**: `VARIABLE`, `PARAMETER`, `FUNCTION`, `STRUCT`, `CLASS`, `FIELD`, `METHOD`.

---

## Struct: `Symbol`

Represents a declared named entity within a lexical scope.

- **Structural Invariant**: `type` must point to a valid `Type` instance; `name` matches the symbol identifier.
- **Memory Footprint**: Approximately 120–140 bytes, including nested `Token` diagnostics data.
- **Lifecycle & Ownership**: Value-stored within `Scope` symbol tables.

### Member Fields
- `std::string name`: Lexical identifier name.
- `const Type* type`: Associated type pointer.
- `SymbolKind kind`: Entity classification.
- `AccessModifier access`: Visibility scope.
- `bool isUsed`: Flag tracking variable/parameter reads (used for warning generation).
- `bool isConstant`: Indicates if the symbol was declared with `const`.
- `bool isInitialized`: Definite assignment tracking flag.
- `Token token`: Source coordinates for diagnostic anchoring.

### Constructors
- `Symbol()`: Initializes an unassigned empty symbol.
- `Symbol(std::string name, const Type* type, SymbolKind kind, Token token, AccessModifier access = AccessModifier::PUBLIC)`: Initializes a symbol with designated type metadata and source coordinates.

---

## Enum Class: `ScopeKind`

Defines the lexical category of a scope frame.

- **Underlying Type**: Implicit `int`.
- **Categorization**: `GLOBAL`, `CLASS`, `FUNCTION`, `BLOCK`.

---

## Class: `Scope`

Represents an isolated lexical scope frame within the symbol table hierarchy.

- **Structural Invariant**: Scopes maintain strong ownership of nested symbols and a shared pointer to their enclosing parent scope.
- **Memory Footprint**: `ScopeKind kind` (4 bytes), `std::shared_ptr<Scope> parent` (16 bytes), `std::unordered_map<std::string, Symbol> symbols` (~56 bytes + nodes).
- **Lifecycle & Ownership**: Managed via `std::shared_ptr`.

### Member Fields
- `ScopeKind kind`: Lexical scope classification.
- `std::shared_ptr<Scope> parent`: Enclosing parent scope (`nullptr` for global scope).
- `std::unordered_map<std::string, Symbol> symbols`: Internal symbol dictionary.

### Constructors & Methods

### `Scope(ScopeKind, std::shared_ptr<Scope>)`
- **Signature**: `explicit Scope(ScopeKind kind, std::shared_ptr<Scope> parent = nullptr)`
- **Parameters & Fields**: Sets scope classification and binds parent scope pointer.

### `getKind() const`
- **Signature**: `ScopeKind getKind() const`
- **Parameters & Fields**: None. Returns `kind`.

### `getParent() const`
- **Signature**: `std::shared_ptr<Scope> getParent() const`
- **Parameters & Fields**: None. Returns `parent`.

### `define(const Symbol&)`
- **Signature**: `bool define(const Symbol& symbol)`
- **Parameters & Fields**: `symbol`: Symbol to register.
- **Technical Purpose & Mechanics**: Checks if `symbol.name` already exists within the local map. If found, returns `false` (redeclaration violation). Otherwise, registers the symbol and returns `true`.

### `resolve(const std::string&) const`
- **Signature**: `std::optional<Symbol> resolve(const std::string& name) const`
- **Parameters & Fields**: `name`: Symbol name to find.
- **Technical Purpose & Mechanics**: Searches `symbols`. If present, returns a copy wrapped in `std::optional`. Otherwise, recursively queries `parent->resolve(name)`. Returns `std::nullopt` if unmapped.

### `resolveSymbol(const std::string&)`
- **Signature**: `Symbol* resolveSymbol(const std::string& name)`
- **Parameters & Fields**: `name`: Symbol name to locate.
- **Technical Purpose & Mechanics**: Searches `symbols` and returns a direct mutable pointer to the stored `Symbol`. If absent, recursively queries `parent->resolveSymbol(name)`. Returns `nullptr` if unmapped. Used to mutate symbol state (e.g., `isUsed`, `isInitialized`).

### `existsInCurrent(const std::string&) const`
- **Signature**: `bool existsInCurrent(const std::string& name) const`
- **Parameters & Fields**: `name`: Symbol name to check.
- **Technical Purpose & Mechanics**: Evaluates whether `name` is present in this scope level without querying parent scopes.

### `getAllSymbols() const`
- **Signature**: `const std::unordered_map<std::string, Symbol>& getAllSymbols() const`
- **Parameters & Fields**: None. Returns a reference to the local symbol map.

---

## Class: `SymbolTable`

Manages the hierarchical stack of active scopes during semantic analysis.

- **Structural Invariant**: `currentScope` points to the innermost active scope; `globalScope` anchors the root.
- **Memory Footprint**: Two `std::shared_ptr<Scope>` instances (32 bytes total).

### Member Fields
- `std::shared_ptr<Scope> currentScope`: Active innermost scope pointer.
- `std::shared_ptr<Scope> globalScope`: Root global scope pointer.

### Constructors & Methods

### `SymbolTable()`
- **Signature**: `SymbolTable()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Instantiates the global scope (`ScopeKind::GLOBAL`) and sets `currentScope = globalScope`.

### `enterScope(ScopeKind)`
- **Signature**: `void enterScope(ScopeKind kind)`
- **Parameters & Fields**: `kind`: Scope classification to instantiate.
- **Technical Purpose & Mechanics**: Pushes a new scope frame by allocating a `Scope(kind, currentScope)` and updating `currentScope`.

### `exitScope()`
- **Signature**: `void exitScope()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Pops the active scope by resetting `currentScope = currentScope->getParent()`. Does not pop the global scope.

### `define(const Symbol&)`
- **Signature**: `bool define(const Symbol& symbol)`
- **Parameters & Fields**: Delegates directly to `currentScope->define(symbol)`.

### `resolve(const std::string&) const`
- **Signature**: `std::optional<Symbol> resolve(const std::string& name) const`
- **Parameters & Fields**: Delegates directly to `currentScope->resolve(name)`.

### `resolveSymbol(const std::string&)`
- **Signature**: `Symbol* resolveSymbol(const std::string& name)`
- **Parameters & Fields**: Delegates directly to `currentScope->resolveSymbol(name)`.

### `existsInCurrentScope(const std::string&) const`
- **Signature**: `bool existsInCurrentScope(const std::string& name) const`
- **Parameters & Fields**: Delegates directly to `currentScope->existsInCurrent(name)`.

### `getCurrentScope() const`, `getGlobalScope() const`
- **Signature**: `std::shared_ptr<Scope> getCurrentScope() const`, `std::shared_ptr<Scope> getGlobalScope() const`
- **Parameters & Fields**: None. Return corresponding scope pointers.

### `isInsideLoop() const`
- **Signature**: `bool isInsideLoop() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Traverses parent scopes upward until encountering a `FUNCTION` boundary. If an enclosing `BLOCK` scope exists, returns `true`; otherwise returns `false`.

---

## Class: `SemanticAnalyzer`

The core semantic validation engine executing passes 1 and 2 over the AST.

- **Structural Invariant**: Maintains an isolated `SymbolTable` and owns dynamic aggregate types created during Pass 1.
- **Lifecycle & Ownership**: Owns custom types via `std::vector<std::unique_ptr<Type>> customTypes`. Owns the diagnostic reporting engine `DiagnosticEngine`.

### Member Fields
- `SymbolTable symbolTable`: Active symbol table manager.
- `DiagnosticEngine diagnostics`: Rust-style diagnostic collector.
- `const Type* currentClass`: Pointer to the enclosing class being validated (`nullptr` if outside a class).
- `const Type* currentFunctionReturnType`: Expected return type of the enclosing subroutine (`nullptr` if in global scope).
- `int loopDepth`: Integer counter tracking nested loop depth.
- `std::vector<std::unique_ptr<Type>> customTypes`: Heap ownership pool for structs and classes.

---

### Internal Helper Methods

### `reportError(DiagnosticCode, const Token&, const std::string&, const std::string&)`
- **Signature**: `void reportError(DiagnosticCode code, const Token& token, const std::string& message, const std::string& hint = "")`
- **Technical Purpose & Mechanics**: Constructs a `SourceSpan(token)` and forwards it to `diagnostics.report()`.

### `reportError(DiagnosticCode, const SourceSpan&, const std::string&, const std::string&)`
- **Signature**: `void reportError(DiagnosticCode code, const SourceSpan& span, const std::string& message, const std::string& hint = "")`
- **Technical Purpose & Mechanics**: Directly forwards span coordinates to `diagnostics.report()`.

### `reportMultiSpan(DiagnosticCode, std::string, const std::vector<SpanLabel>&, std::string)`
- **Signature**: `void reportMultiSpan(DiagnosticCode code, std::string message, const std::vector<SpanLabel>& labels, std::string hint = "")`
- **Technical Purpose & Mechanics**: Forwards a multi-location error to `diagnostics.reportMultiSpan()`.

### `getVisibleSymbolNames() const`
- **Signature**: `std::vector<std::string> getVisibleSymbolNames() const`
- **Technical Purpose & Mechanics**: Traverses the active scope chain upward, collecting variable, parameter, and function names for typo correction suggestions.

### `getVisibleTypeNames() const`
- **Signature**: `std::vector<std::string> getVisibleTypeNames() const`
- **Technical Purpose & Mechanics**: Collects all primitive types and user-defined structs/classes visible in the current scope for typo suggestions.

### `resolveTypeFromToken(const Token&)`
- **Signature**: `const Type* resolveTypeFromToken(const Token& typeToken)`
- **Technical Purpose & Mechanics**: Resolves a type lexeme into a canonical `const Type*`:
    1. Recursively unwraps trailing nullable `?` via `Type::makeNullable()`.
    2. Recursively unwraps trailing array `[]` via `Type::makeArray()`.
    3. Matches primitive names (`int`, `float`, `char`, `string`, `bool`, `void`).
    4. Looks up user-defined types in `symbolTable`. If not found, emits `E0007_UNKNOWN_TYPE_NAME` with a typo hint and returns `Type::getError()`.

### `isLValue(const Expr&)`
- **Signature**: `static bool isLValue(const Expr& expr)`
- **Technical Purpose & Mechanics**: Returns `true` if `expr` is a `VariableExpr`, `MemberAccessExpr`, or `ArrayAccessExpr`.

### `getExprToken(const Expr&)`, `getExprSpan(const Expr&)`
- **Signature**: `static Token getExprToken(const Expr& expr)`, `static SourceSpan getExprSpan(const Expr& expr)`
- **Technical Purpose & Mechanics**: Extracts the anchor token or source span from an expression variant via `std::visit`.

### `getStmtToken(const Stmt&)`, `getStmtSpan(const Stmt&)`
- **Signature**: `static Token getStmtToken(const Stmt& stmt)`, `static SourceSpan getStmtSpan(const Stmt& stmt)`
- **Technical Purpose & Mechanics**: Extracts the primary anchor token or source span from a statement variant via `std::visit`.

### `hasGuaranteedReturn(const Stmt&)`
- **Signature**: `bool hasGuaranteedReturn(const Stmt& stmt)`
- **Technical Purpose & Mechanics**: Verifies if a statement guarantees termination via `return`:
    - `ReturnStmt`: Returns `true`.
    - `BlockStmt`: Evaluates statements sequentially. If a statement terminates, checks for unreachable trailing statements (`W0003_UNREACHABLE_CODE`) and returns `true`.
    - `IfStmt`: Returns `true` if and only if an `elseBranch` exists and `thenBranch`, all `elifBranches`, and `elseBranch` guarantee return.
    - Fallback: Returns `false`.

---

### Pipeline Orchestration & Passes

### `setSource(std::string, std::string)`
- **Signature**: `void setSource(std::string filename, std::string source)`
- **Parameters & Fields**:
    - `filename`: Source file path or identifier.
    - `source`: Complete in-memory text buffer of the input file.
- **Technical Purpose & Mechanics**: Forwards the source buffer and file path to the internal `DiagnosticEngine` instance, precomputing newline offsets to support source snippet rendering.
- **Dependencies & Invocations**: Invoked in `main.cpp` prior to invoking `analyze()`.

### `getDiagnostics() const`
- **Signature**: `const DiagnosticEngine& getDiagnostics() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Provides constant reference access to the accumulated diagnostic collection.
- **Dependencies & Invocations**: Invoked by compilation drivers to inspect warnings, errors, and print diagnostics.

### `hasErrors() const`
- **Signature**: `bool hasErrors() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Queries the internal `DiagnosticEngine`, returning `true` if any recorded diagnostic has `DiagnosticSeverity::ERROR`.
- **Dependencies & Invocations**: Invoked by `analyze()` to compute compilation success.

### `analyze(const std::vector<Stmt>&)`
- **Signature**: `bool analyze(const std::vector<Stmt>& ast)`
- **Parameters & Fields**: `ast`: Top-level AST statements.
- **Technical Purpose & Mechanics**: Clears diagnostics, executes `collectDeclarations(ast)` (Pass 1), executes `typeCheck(ast)` (Pass 2), and returns `!hasErrors()`.

### `collectDeclarations(const std::vector<Stmt>&)`
- **Signature**: `void collectDeclarations(const std::vector<Stmt>& ast)`
- **Parameters & Fields**: `ast`: Top-level AST statements.
- **Technical Purpose & Mechanics**:
    - Sub-pass 1: Registers aggregate types (`StructDeclStmt`, `ClassDeclStmt`) into `symbolTable` and stores them in `customTypes`. Rejects duplicate declarations (`E0004`).
    - Sub-pass 2: Populates struct fields. Resolves class superclasses while detecting cyclic inheritance (`E0035`). Populates class fields and methods. Registers global function prototypes.

### `typeCheck(const std::vector<Stmt>&)`
- **Signature**: `void typeCheck(const std::vector<Stmt>& ast)`
- **Parameters & Fields**: `ast`: Top-level AST statements.
- **Technical Purpose & Mechanics**: Iterates through `ast` invoking `execute(stmt)`. Checks global symbols for unused variables, emitting `W0001_UNUSED_VARIABLE` for unreferenced symbols not prefixed with `_`.

### `evaluate(const Expr&)`
- **Signature**: `const Type* evaluate(const Expr& expr)`
- **Technical Purpose & Mechanics**: Evaluates an expression by dispatching visitor execution via `std::visit(*this, expr.as)`.

### `execute(const Stmt&)`
- **Signature**: `void execute(const Stmt& stmt)`
- **Technical Purpose & Mechanics**: Executes a statement by dispatching visitor execution via `std::visit(*this, stmt.as)`.

---

### Expression Visitors

### `operator()(const std::unique_ptr<LiteralExpr>&)`
- **Signature**: `const Type* operator()(const std::unique_ptr<LiteralExpr>& node)`
- **Technical Purpose & Mechanics**: Maps literal tokens to their respective types. Checks 32-bit bounds on integer literals, emitting `E0006_TYPE_MISMATCH` if values exceed $[-2147483648, 2147483647]$.

### `operator()(const std::unique_ptr<VariableExpr>&)`
- **Signature**: `const Type* operator()(const std::unique_ptr<VariableExpr>& node)`
- **Technical Purpose & Mechanics**: Resolves the variable in `symbolTable`. If missing, emits `E0001_UNDECLARED_VARIABLE` with typo matching. Marks the symbol as used (`isUsed = true`). Enforces definite assignment: if `!isInitialized`, emits `E0001_UNDECLARED_VARIABLE`. Returns `sym->type`.

### `operator()(const std::unique_ptr<ThisExpr>&)`
- **Signature**: `const Type* operator()(const std::unique_ptr<ThisExpr>& node)`
- **Technical Purpose & Mechanics**: Ensures `currentClass != nullptr`; emits `E0026_THIS_OUTSIDE_CLASS` if used outside class methods. Returns `currentClass`.

### `operator()(const std::unique_ptr<BinaryExpr>&)`
- **Signature**: `const Type* operator()(const std::unique_ptr<BinaryExpr>& node)`
- **Technical Purpose & Mechanics**:
    - Assignment (`=`): Validates `isLValue(node->left)`. Checks `const` immutability (`E0003`). Marks variables as initialized (`isInitialized = true`). Verifies `rightType->isAssignableTo(leftType)`.
    - Compound Assignment (`+=`, `-=`, etc.): Enforces lvalue requirements and numeric types (permits `+=` for string concatenation).
    - Arithmetic (`+`, `-`, `*`, `/`, `%`): Resolves string concatenation or numeric arithmetic promotion (`int` vs `float`).
    - Equality (`==`, `!=`): Checks bidirectional assignability.
    - Relational (`<`, `<=`, `>`, `>=`): Enforces numeric operands and returns `Type::getBool()`.
    - Logical (`&&`, `||`): Enforces boolean operands (`E0009`).
    - Type Check (`is`): Verifies an inheritance or subtyping relationship between operands.

### `operator()(const std::unique_ptr<RangeExpr>&)`
- **Signature**: `const Type* operator()(const std::unique_ptr<RangeExpr>& node)`
- **Technical Purpose & Mechanics**: Asserts that `start` and `end` bounds are integers (`E0012`). Returns `Type::makeArray(Type::getInt())`.

### `operator()(const std::unique_ptr<UnaryExpr>&)`
- **Signature**: `const Type* operator()(const std::unique_ptr<UnaryExpr>& node)`
- **Technical Purpose & Mechanics**: Validates boolean operands for `!`, numeric operands for unary `-`/`+`, and numeric lvalue targets for `++`/`--`.

### `operator()(const std::unique_ptr<GroupingExpr>&)`
- **Signature**: `const Type* operator()(const std::unique_ptr<GroupingExpr>& node)`
- **Technical Purpose & Mechanics**: Recursively evaluates and returns the type of `node->expression`.

### `operator()(const std::unique_ptr<CallExpr>&)`
- **Signature**: `const Type* operator()(const std::unique_ptr<CallExpr>& node)`
- **Technical Purpose & Mechanics**: If the callee is a struct or class, treats the invocation as an instantiation and returns the callee type. If a function, validates argument count (`E0032`) and type assignability (`E0033`), returning `returnType`. Otherwise emits `E0031_NOT_CALLABLE`.

### `operator()(const std::unique_ptr<MemberAccessExpr>&)`
- **Signature**: `const Type* operator()(const std::unique_ptr<MemberAccessExpr>& node)`
- **Technical Purpose & Mechanics**: Rejects dereferences on nullable types (`E0030`) and non-aggregate primitives. Searches fields and methods across the class and its superclass hierarchy. Verifies visibility constraints (`PUBLIC`, `PROTECTED`, `PRIVATE`) against `currentClass`, emitting `E0028_PRIVATE_MEMBER_ACCESS` on violations.

### `operator()(const std::unique_ptr<ArrayAccessExpr>&)`
- **Signature**: `const Type* operator()(const std::unique_ptr<ArrayAccessExpr>& node)`
- **Technical Purpose & Mechanics**: Asserts that index is an integer (`E0018`). Rejects indexing on nullable arrays (`E0015`). Asserts array type (`E0017`) and returns `arrayType->elementType`.

### `operator()(const std::unique_ptr<ArrayLiteralExpr>&)`
- **Signature**: `const Type* operator()(const std::unique_ptr<ArrayLiteralExpr>& node)`
- **Technical Purpose & Mechanics**: Infers the common element type across all elements: supports subtype widening and promotes `T` to `T?` if `null` literals are present. Returns `Type::makeArray(commonType)`.

### `operator()(const std::unique_ptr<TernaryExpr>&)`
- **Signature**: `const Type* operator()(const std::unique_ptr<TernaryExpr>& node)`
- **Technical Purpose & Mechanics**: Asserts that `condition` evaluates to `bool` (`E0009`). Verifies that true and false branches yield compatible types (`E0011`). Returns the resolved branch type.

### `operator()(const std::unique_ptr<CastExpr>&)`
- **Signature**: `const Type* operator()(const std::unique_ptr<CastExpr>& node)`
- **Technical Purpose & Mechanics**: Resolves target type. Validates identity casts, numeric conversions (`int` $\leftrightarrow$ `float`), and class hierarchy upcasting/downcasting. Emits `E0006_TYPE_MISMATCH` on invalid casts.

---

### Statement Visitors

### `operator()(const std::unique_ptr<ExpressionStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<ExpressionStmt>& node)`
- **Technical Purpose & Mechanics**: Evaluates the contained expression via `evaluate()`.

### `operator()(const std::unique_ptr<VarDeclStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<VarDeclStmt>& node)`
- **Technical Purpose & Mechanics**: Rejects `void` variable declarations (`E0006`). Checks for duplicate local declarations (`E0004`). Type-checks initializers against declared types. Defines a new `Symbol` tracking initialization state and `const` immutability.

### `operator()(const std::unique_ptr<BlockStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<BlockStmt>& node)`
- **Technical Purpose & Mechanics**: Enters `ScopeKind::BLOCK`, executes all statements sequentially, inspects the scope for unused local variables (`W0001`), and exits the scope.

### `operator()(const std::unique_ptr<IfStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<IfStmt>& node)`
- **Technical Purpose & Mechanics**:
    - Validates boolean conditions for root and `elif` branches (`E0009`).
    - Analyzes conditions (`var != null`) and promotes nullable variables to non-nullable types in the guarded `then` block scope.
    - Analyzes inverted conditions (`var == null`) to promote variables in the `else` block scope.
    - Type-checks all branches in isolated block scopes.

### `operator()(const std::unique_ptr<WhileStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<WhileStmt>& node)`
- **Technical Purpose & Mechanics**: Asserts boolean condition (`E0009`), increments `loopDepth`, executes the body statement, and decrements `loopDepth`.

### `operator()(const std::unique_ptr<ForStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<ForStmt>& node)`
- **Technical Purpose & Mechanics**: Asserts that the iterable evaluates to an array (`E0010`). Increments `loopDepth`, enters a loop block scope, registers the iterator variable bound to the array's element type, executes the loop body, and exits the scope.

### `operator()(const std::unique_ptr<FunctionDeclStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<FunctionDeclStmt>& node)`
- **Technical Purpose & Mechanics**: Enters `ScopeKind::FUNCTION`, binds parameters into the scope, checks default parameter ordering (`E0034`), executes the body block, verifies guaranteed return paths for non-void functions (`E0025`), checks for unused parameters (`W0002`), and exits the scope.

### `operator()(const std::unique_ptr<StructDeclStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<StructDeclStmt>& node)`
- **Technical Purpose & Mechanics**: No-op during Pass 2; layouts are registered during Pass 1.

### `operator()(const std::unique_ptr<ClassDeclStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<ClassDeclStmt>& node)`
- **Technical Purpose & Mechanics**: Sets `currentClass`, enters `ScopeKind::CLASS`, executes member declarations (methods), and resets `currentClass` upon scope exit.

### `operator()(const std::unique_ptr<ReturnStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<ReturnStmt>& node)`
- **Technical Purpose & Mechanics**: Asserts execution within a function (`E0021`). Rejects returning expressions from `void` functions (`E0024`). Rejects empty returns from non-void functions (`E0023`). Asserts return value type compatibility (`E0022`).

### `operator()(const std::unique_ptr<BreakStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<BreakStmt>& node)`
- **Technical Purpose & Mechanics**: Verifies `loopDepth > 0`; emits `E0019_BREAK_OUTSIDE_LOOP` if outside loops.

### `operator()(const std::unique_ptr<ContinueStmt>&)`
- **Signature**: `void operator()(const std::unique_ptr<ContinueStmt>& node)`
- **Technical Purpose & Mechanics**: Verifies `loopDepth > 0`; emits `E0020_CONTINUE_OUTSIDE_LOOP` if outside loops.

---
# Compiler Diagnostic Engine & Reporting Tooling

The Diagnostic Engine provides Rust-style diagnostic rendering, line-offset precomputation, multi-span source annotation, and string edit distance calculations for compiler error reporting.

### Rendering Architecture & Source Indexing
`DiagnosticEngine` caches the source text and precomputes byte offsets for each newline (`buildLineIndex()`), allowing $O(1)$ random access to any line. Tabs (`\t`) are expanded to 4 spaces during line extraction to align carets and underlines with terminal columns. Diagnostics support multi-span labels, distinguishing between primary error anchors (`^` in red) and secondary context spans (`-` in cyan).

```
error[E0006]: Cannot assign value of type 'string' to target of type 'int'.
  --> main.wlf:12:9
   |
12 |     int x = "hello"
   |         -   ^^^^^^^ types must match or be implicitly convertible
   |         |
   |         declared as 'int' here
   = help: ensure expression type matches declared variable type
```

---

## Enum Class: `DiagnosticSeverity`

Defines the severity level of an emitted diagnostic.

- **Underlying Type**: Implicit `int`.
- **Categorization**: `ERROR`, `WARNING`, `NOTE`, `HELP`.

---

## Struct: `SourceSpan`

Represents a coordinate range within the source text.

- **Structural Invariant**: 1-based indexing for lines and columns; length corresponds to character span width.
- **Memory Footprint**: `int line` (4 bytes), `int column` (4 bytes), `size_t length` (8 bytes) = 16 bytes.

### Member Fields
- `int line`: Line index (1-based).
- `int column`: Column offset (1-based).
- `size_t length`: Character count to underline.

### Constructors
- `SourceSpan()`: Default constructor (`line = 1`, `column = 1`, `length = 1`).
- `SourceSpan(int l, int c, size_t len)`: Direct coordinate initialization.
- `SourceSpan(const Token& tok)`: Extracts line, column, and length from `tok.lexeme`.

---

## Struct: `SpanLabel`

Associates an inline annotation message with a `SourceSpan`.

- **Structural Invariant**: `span` defines the region; `isPrimary` controls whether the label uses primary (`^`) or secondary (`-`) markers.
- **Memory Footprint**: `SourceSpan span` (16 bytes), `std::string label` (24–32 bytes), `bool isPrimary` (1 byte) + padding = ~48–56 bytes.

### Member Fields
- `SourceSpan span`: Target coordinate span.
- `std::string label`: Contextual message rendered beside the caret markers.
- `bool isPrimary`: True for primary error spans; false for contextual notes.

### Constructors
- `SpanLabel()`: Default constructor.
- `SpanLabel(SourceSpan s, std::string lbl = "", bool primary = true)`: Initializes from a span.
- `SpanLabel(const Token& tok, std::string lbl = "", bool primary = true)`: Initializes from a token.

---
## Enum Class: DiagnosticCode

Defines the complete enumerated taxonomy of compiler diagnostics, categorizing fatal breaking violations (`E`), non-breaking static analysis warnings (`W`), and contextual auxiliary notes (`N`).

- **Underlying Type**: Implicit `int`.
- **Structural Invariant**: Every diagnostic reported by the front-end carries an explicit tag from this enumeration.
- **Categorization**:

### Error Codes (Breaking Semantic Violations)
- **`E0001_UNDECLARED_VARIABLE` (`"E0001"`):** Emitted when an identifier is referenced in an expression but does not resolve to an active symbol in the current scope chain.
- **`E0002_UNINITIALIZED_VARIABLE` (`"E0002"`):** Emitted when a variable is read before being assigned an initial value.
- **`E0003_CANNOT_ASSIGN_CONSTANT` (`"E0003"`):** Emitted when an assignment target resolves to a variable declared with `const`.
- **`E0004_DUPLICATE_DECLARATION` (`"E0004"`):** Emitted when a variable, function, class, struct, or member is re-declared within the same lexical scope.
- **`E0005_INVALID_ASSIGNMENT_TARGET` (`"E0005"`):** Emitted when the left-hand side of an assignment or increment/decrement is not an assignable lvalue.
- **`E0006_TYPE_MISMATCH` (`"E0006"`):** Emitted when a value cannot be implicitly converted or assigned to the target type, or when an integer literal exceeds 32-bit signed limits.
- **`E0007_UNKNOWN_TYPE_NAME` (`"E0007"`):** Emitted when a type specifier token does not resolve to a primitive, struct, or class type in scope.
- **`E0008_INTEGER_OVERFLOW` (`"E0008"`):** Emitted when a numeric literal exceeds maximum representable bounds.
- **`E0009_NON_BOOLEAN_CONDITION` (`"E0009"`):** Emitted when an `if`, `while`, or ternary condition evaluates to a non-boolean type.
- **`E0010_INVALID_OPERATOR_OPERANDS` (`"E0010"`):** Emitted when an operator receives operands of incompatible or unsupported types.
- **`E0011_INCOMPATIBLE_TERNARY_BRANCHES` (`"E0011"`):** Emitted when the true and false branch expressions of a ternary operator produce incompatible types.
- **`E0012_INVALID_RANGE_BOUNDS` (`"E0012"`):** Emitted when range boundaries (`start..end`) do not evaluate to integer expressions.
- **`E0013_NULL_ASSIGNED_TO_NON_NULLABLE` (`"E0013"`):** Emitted when literal `null` is assigned to a non-nullable type (`T`).
- **`E0014_NULLABLE_MEMBER_DEREFERENCE` (`"E0014"`):** Emitted when accessing fields or methods on a nullable receiver without a guarding non-null assertion.
- **`E0015_NULLABLE_ARRAY_SUBSCRIPT` (`"E0015"`):** Emitted when indexing into an array variable of nullable type (`T[]?`) without a null check.
- **`E0016_ARRAY_ELEMENT_TYPE_MISMATCH` (`"E0016"`):** Emitted when elements of an array literal cannot unify to a common base type.
- **`E0017_INVALID_SUBSCRIPT_TARGET` (`"E0017"`):** Emitted when the subscript operator `[]` is applied to a non-array expression.
- **`E0018_INVALID_SUBSCRIPT_INDEX` (`"E0018"`):** Emitted when an array subscript index does not evaluate to an integer.
- **`E0019_BREAK_OUTSIDE_LOOP` (`"E0019"`):** Emitted when a `break` statement occurs outside a `while` or `for` loop body.
- **`E0020_CONTINUE_OUTSIDE_LOOP` (`"E0020"`):** Emitted when a `continue` statement occurs outside a `while` or `for` loop body.
- **`E0021_RETURN_OUTSIDE_FUNCTION` (`"E0021"`):** Emitted when a `return` statement is encountered in global scope.
- **`E0022_RETURN_TYPE_MISMATCH` (`"E0022"`):** Emitted when the returned expression type is not assignable to the declared function return type.
- **`E0023_MISSING_RETURN_VALUE` (`"E0023"`):** Emitted when a non-void function executes an empty `return` without a value.
- **`E0024_RETURN_IN_VOID_FUNCTION` (`"E0024"`):** Emitted when a function with `void` return type returns an expression.
- **`E0025_NOT_ALL_PATHS_RETURN` (`"E0025"`):** Emitted when a non-void function fails to guarantee a return on all possible execution paths.
- **`E0026_THIS_OUTSIDE_CLASS` (`"E0026"`):** Emitted when `this` is used outside of a class method.
- **`E0027_THIS_IN_STATIC_METHOD` (`"E0027"`):** Emitted when `this` is referenced inside a static method.
- **`E0028_PRIVATE_MEMBER_ACCESS` (`"E0028"`):** Emitted when attempting to access a `private` member from outside the declaring class.
- **`E0029_PROTECTED_MEMBER_ACCESS` (`"E0029"`):** Emitted when accessing a `protected` member from an unrelated class.
- **`E0030_MEMBER_NOT_FOUND` (`"E0030"`):** Emitted when accessing a member that does not exist on the target aggregate type.
- **`E0031_NOT_CALLABLE` (`"E0031"`):** Emitted when an invocation `()` is attempted on an expression that is neither a function, class, nor struct.
- **`E0032_ARGUMENT_COUNT_MISMATCH` (`"E0032"`):** Emitted when the number of arguments provided to a function exceeds parameter capacity.
- **`E0033_ARGUMENT_TYPE_MISMATCH` (`"E0033"`):** Emitted when an argument expression is not assignable to its corresponding parameter contract.
- **`E0034_DEFAULT_PARAM_ORDER` (`"E0034"`):** Emitted when a parameter without a default value follows a parameter with a default value.
- **`E0035_CYCLIC_INHERITANCE` (`"E0035"`):** Emitted when a class hierarchy contains an inheritance cycle (e.g., $A \to B \to A$).
- **`E0036_CANNOT_INSTANTIATE_ABSTRACT_CLASS` (`"E0036"`):** Emitted when attempting to instantiate an abstract class.
- **`E0037_UNIMPLEMENTED_ABSTRACT_METHOD` (`"E0037"`):** Emitted when a concrete class fails to provide an implementation for an inherited abstract method.
- **`E0038_INVALID_OVERRIDE` (`"E0038"`):** Emitted when a method marked `override` does not match an inherited virtual method signature.
- **`E0039_CANNOT_EXTEND_FINAL_CLASS` (`"E0039"`):** Emitted when attempting to subclass a non-extendable class.
- **`E0040_DUPLICATE_CLASS_MEMBER` (`"E0040"`):** Emitted when duplicate fields or methods share the same identifier within a class.
- **`E0041_VOID_VARIABLE_DECLARATION` (`"E0041"`):** Emitted when a variable is declared with type `void`.

### Warning Codes (Non-Breaking Code Smells)
- **`W0001_UNUSED_VARIABLE` (`"W0001"`):** Emitted when a declared variable is never read within its scope.
- **`W0002_UNUSED_PARAMETER` (`"W0002"`):** Emitted when a formal parameter is never read within the subroutine body.
- **`W0003_UNREACHABLE_CODE` (`"W0003"`):** Emitted when statements follow a terminating jump (`return`) in a block.
- **`W0004_VARIABLE_SHADOWING` (`"W0004"`):** Emitted when an inner variable shadows a variable declared in an enclosing scope.
- **`W0005_NUMERIC_PRECISION_LOSS` (`"W0005"`):** Emitted when float values are narrowed to integers or truncated.
- **`W0006_UNUSED_IMPORT` (`"W0006"`):** Emitted when an imported module or symbol is unreferenced.
- **`W0007_EMPTY_BLOCK` (`"W0007"`):** Emitted when a block statement contains no operations.
- **`W0008_STATIC_CALLED_ON_INSTANCE` (`"W0008"`):** Emitted when a static method is invoked via an instance reference.

### Note Codes (Contextual Diagnostic Annotations)
- **`N0001_PREVIOUS_DECLARATION` (`"N0001"`):** Highlights the location of the original symbol definition during a duplicate declaration conflict.
- **`N0002_HELP_SUGGESTION` (`"N0002"`):** Auxiliary suggestion rendered to propose code fixes or alternative identifiers.
- **`N0003_TYPE_INFERRED_HERE` (`"N0003"`):** Secondary location anchor showing where a conflicting type was originally inferred.

---

## Standalone Function: `diagnosticCodeToString`

- **Signature**: `inline std::string diagnosticCodeToString(DiagnosticCode code)`
- **Parameters & Fields**:
    - `code`: Diagnostic code enum value.
- **Technical Purpose & Mechanics**: Executes a switch statement returning the textual code identifier (e.g., `"E0001"`, `"W0003"`). Defaults to `"D9999"`.
- **Dependencies & Invocations**: Invoked by `DiagnosticEngine::print()` when formatting diagnostic banners.

---

## Class: `DiagnosticEngine`

Manages diagnostic collection, string distance calculations, and terminal formatting.

- **Structural Invariant**: `lineOffsets` contains valid byte offsets into `sourceCode`.
- **Memory Footprint**: Holds `sourceCode`, `filename`, diagnostic records, and the line offset vector (~100–120 bytes plus allocations).

### Member Fields
- `std::string sourceCode`: Cached raw source text.
- `std::string filename`: Path or identifier of the source file.
- `std::vector<Diagnostic> diagnostics`: Recorded diagnostics collection.
- `std::vector<size_t> lineOffsets`: Precomputed byte offsets of every line start.

---

### Internal Helpers & String Matching

### `buildLineIndex()`
- **Signature**: `void buildLineIndex()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Clears `lineOffsets`, pushes offset `0` for line 1, and iterates through `sourceCode` appending `i + 1` for every newline character (`\n`).
- **Dependencies & Invocations**: Invoked internally by `setSource()`.

### `extractLine(int) const`
- **Signature**: `std::string extractLine(int lineNumber) const`
- **Parameters & Fields**:
    - `lineNumber`: 1-based line number.
- **Technical Purpose & Mechanics**: Retrieves line boundaries from `lineOffsets`, slices the substring, strips trailing `\r`, expands tabs (`\t`) to 4 spaces, and returns the line string.
- **Dependencies & Invocations**: Invoked by `print()` during source snippet rendering.

### `editDistance(const std::string&, const std::string&) const`
- **Signature**: `size_t editDistance(const std::string& a, const std::string& b) const`
- **Parameters & Fields**:
    - `a`, `b`: Strings to compare.
- **Technical Purpose & Mechanics**: Computes the Levenshtein distance using dynamic programming with an $(m+1) \times (n+1)$ table, tracking insertions, deletions, and substitutions.
- **Dependencies & Invocations**: Invoked by `findClosestMatch()`.

### `findClosestMatch(const std::string&, const std::vector<std::string>&) const`
- **Signature**: `std::string findClosestMatch(const std::string& target, const std::vector<std::string>& candidates) const`
- **Parameters & Fields**:
    - `target`: Misspelled identifier.
    - `candidates`: Vector of visible symbol or type names.
- **Technical Purpose & Mechanics**: Filters out candidates where length difference exceeds 2, calculates `editDistance`, and returns the closest candidate with edit distance $\le 2$.
- **Dependencies & Invocations**: Invoked by `SemanticAnalyzer` when proposing "did you mean?" hints.

---

### Diagnostic Reporting Interface

### `setSource(std::string, std::string)`
- **Signature**: `void setSource(std::string file, std::string source)`
- **Parameters & Fields**:
    - `file`: Path or identifier of the source file.
    - `source`: Complete raw source code text.
- **Technical Purpose & Mechanics**: Stores `file` into `filename`, moves `source` into `sourceCode`, and invokes `buildLineIndex()` to precompute line start byte offsets for $O(1)$ line extraction.
- **Dependencies & Invocations**: Invokes `buildLineIndex()`. Called by `SemanticAnalyzer::setSource()`.

### `report(DiagnosticCode, const SourceSpan&, std::string, std::string)`
- **Signature**: `void report(DiagnosticCode code, const SourceSpan& span, std::string message, std::string hint = "")`
- **Parameters & Fields**:
    - `code`: Diagnostic error code (`DiagnosticCode`).
    - `span`: Primary source coordinate anchor.
    - `message`: Error description.
    - `hint`: Optional suggested remediation.
- **Technical Purpose & Mechanics**: Constructs a `Diagnostic` record with `DiagnosticSeverity::ERROR`, attaches a single primary `SpanLabel`, and appends it to `diagnostics`.
- **Dependencies & Invocations**: Pushes to `diagnostics`.

### `reportMultiSpan(DiagnosticCode, std::string, const std::vector<SpanLabel>&, std::string)`
- **Signature**: `void reportMultiSpan(DiagnosticCode code, std::string message, const std::vector<SpanLabel>& labels, std::string hint = "")`
- **Parameters & Fields**:
    - `code`: Diagnostic error code.
    - `message`: Error description.
    - `labels`: Collection of source spans (both primary and secondary context).
    - `hint`: Optional suggested remediation.
- **Technical Purpose & Mechanics**: Appends a multi-location `Diagnostic` record with `DiagnosticSeverity::ERROR`, preserving all context spans.
- **Dependencies & Invocations**: Pushes to `diagnostics`.

### `reportWarning(DiagnosticCode, const SourceSpan&, std::string, std::string)`
- **Signature**: `void reportWarning(DiagnosticCode code, const SourceSpan& span, std::string message, std::string hint = "")`
- **Parameters & Fields**:
    - `code`: Diagnostic warning code.
    - `span`: Source coordinate anchor.
    - `message`: Warning description.
    - `hint`: Optional remediation suggestion.
- **Technical Purpose & Mechanics**: Constructs a `Diagnostic` record with `DiagnosticSeverity::WARNING` and appends it to `diagnostics`. Does not prevent compilation success.
- **Dependencies & Invocations**: Pushes to `diagnostics`.

### `hasErrors() const`
- **Signature**: `bool hasErrors() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Iterates through `diagnostics`, returning `true` if any diagnostic has `severity == DiagnosticSeverity::ERROR`.
- **Dependencies & Invocations**: Called by `SemanticAnalyzer::hasErrors()`.

### `errorCount() const`
- **Signature**: `size_t errorCount() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns the total number of recorded diagnostics with `DiagnosticSeverity::ERROR`.
- **Dependencies & Invocations**: Called by compiler drivers (`main.cpp`) for reporting summaries.

### `warningCount() const`
- **Signature**: `size_t warningCount() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns the total number of recorded diagnostics with `DiagnosticSeverity::WARNING`.
- **Dependencies & Invocations**: Called by compiler drivers (`main.cpp`) for reporting summaries.

### `getDiagnostics() const`
- **Signature**: `const std::vector<Diagnostic>& getDiagnostics() const`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Returns an immutable reference to the internal `diagnostics` vector.
- **Dependencies & Invocations**: Consumed by test suites and IDE integration adapters.

### `clear()`
- **Signature**: `void clear()`
- **Parameters & Fields**: None.
- **Technical Purpose & Mechanics**: Clears all elements from `diagnostics`.
- **Dependencies & Invocations**: Called at the beginning of `SemanticAnalyzer::analyze()`.

### `print(std::ostream&, bool) const`
- **Signature**: `void print(std::ostream& out, bool ideMode = false) const`
- **Parameters & Fields**:
    - `out`: Output stream to render diagnostics into.
    - `ideMode`: When `true`, emits single-line diagnostics compatible with IDE problem matchers.
- **Technical Purpose & Mechanics**:
    - If `ideMode` is true: Formats single-line outputs as `filename:line:col: severity[code]: message\n`.
    - Otherwise: Formats Rust-style diagnostic boxes using ANSI terminal colors, line numbers, gutter bars (`|`), extracted source lines via `extractLine()`, column-aligned markers (`^` red for primary, `-` cyan for secondary), and help footers (`= help:`).
- **Dependencies & Invocations**: Invokes `diagnosticCodeToString()`, `extractLine()`. Consumed by compilation drivers to render fatal errors and warnings.

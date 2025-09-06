# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a C compiler implementation that generates LLVM IR (Intermediate Representation) from C source code. The project uses bison for parsing, flex for lexical analysis, and outputs LLVM IR that can be compiled with LLVM tools.

**Architecture**: Classic compiler design with separate lexing, parsing, AST construction, and code generation phases.

## Build System

### Core Commands
```bash
make              # Build compiler (generates ./ccompiler)
make clean        # Remove generated files and objects  
make debug        # Build with debug symbols and -DDEBUG
make test         # Run test suite on sample C programs
make validate     # Validate generated LLVM IR syntax
make check-deps   # Verify bison, flex, g++, llvm-config availability
```

### Development Workflow
```bash
make run INPUT=file.c OUTPUT=file.ll    # Compile C to LLVM IR
make run INPUT=file.c                   # Output to stdout
make debug && ./ccompiler -v -a file.c  # Debug mode with verbose AST output
```

### Generated Files
- `grammar.tab.cpp/.hpp` - Parser from bison
- `lex.yy.c` - Lexer from flex  
- `grammar.output` - Parser state machine (bison -v)
- `*.o` - Object files

## Code Architecture

### Core Components

**AST System (`src/ast.h`, `src/ast.cpp`)**
- `ASTNode`: Unified node structure with type-tagged unions for all C constructs
- `TypeInfo`: Type representation with qualifiers, storage classes, pointer levels
- `Symbol`: Symbol table entries with scope and type information
- 47 AST node types covering full C language (expressions, statements, declarations)

**Code Generation (`src/codegen.h`, `src/codegen.cpp`)**  
- `CodeGenContext`: Central state for LLVM IR generation
- Register allocation with `next_reg_id` counter
- Basic block management with labels and control flow
- Symbol table management (global/local scopes)
- Type system mapping C types to LLVM types

**Parser (`src/grammar.y`)**
- Full C89/C90 grammar specification
- Operator precedence and associativity rules
- AST construction actions for all productions
- Error recovery mechanisms

**Lexer (`src/lexer.l`)**
- Complete C tokenization (keywords, operators, literals)
- Line/column tracking for error reporting
- Comment handling and whitespace management

**Main Driver (`src/main.cpp`)**
- Command-line option parsing (`-v`, `-a`, `-o`, `-h`)
- File I/O management and error handling
- Integration of lexer → parser → codegen pipeline

### Data Flow

```
C Source → Lexer (flex) → Tokens → Parser (bison) → AST → Code Generator → LLVM IR
```

1. **Lexical Analysis**: `src/lexer.l` tokenizes C source into terminal symbols
2. **Syntax Analysis**: `src/grammar.y` builds AST from tokens using grammar rules
3. **Code Generation**: `src/codegen.cpp` traverses AST and emits LLVM IR instructions
4. **Output**: LLVM IR text format ready for `llc` or `clang` backend

### Key Design Patterns

**Type-Tagged Unions**: AST nodes use discriminated unions with `ASTNodeType` enum for type safety
**Visitor Pattern**: Code generation traverses AST with type-specific generation functions
**Symbol Tables**: Hierarchical scoping with global/local symbol management
**Register Allocation**: SSA-form register naming with sequential ID assignment

## Testing Infrastructure

### Test Files
- `tests/*.c` - C source test cases
- `tests/*.ll` - Expected/generated LLVM IR output
- Test categories: basic expressions, functions, control flow, recursion, data types

### Validation
```bash
make validate     # Check LLVM IR syntax with llvm-as
llc tests/file.ll # Compile to machine code
lli tests/file.ll # Execute LLVM IR directly
```

## Development Notes

### Code Style
- Snake_case for functions and variables
- ALL_CAPS for token names and enum constants
- Tab indentation (4-space equivalent)
- C-style `/* */` comments in grammar files

### Debugging Features
- `-v` flag: Verbose compilation output
- `-a` flag: Dump AST structure
- `-d` flag: Debug mode with detailed tracing
- `make debug`: Build with debug symbols and assertions

### LLVM Integration
- Optional LLVM dependency detection via `llvm-config`
- Graceful fallback when LLVM tools unavailable
- Generated IR compatible with LLVM 3.0+ through current versions

### Extension Points
- New AST node types: Add to `ASTNodeType` enum and update visitor functions
- New operators: Add tokens to lexer, grammar rules to parser, generation logic
- New C constructs: Follow the lexer → grammar → AST → codegen pattern
- Optimization passes: Insert between AST construction and code generation
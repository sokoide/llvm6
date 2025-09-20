# C-to-LLVM IR Compiler API Documentation

**Generated:** January 2025  
**Version:** 1.0  
**Coverage:** Complete API Reference

## Overview

This document provides comprehensive API documentation for the C-to-LLVM IR compiler. The compiler follows a clean 4-phase architecture: Lexical Analysis → Syntax Analysis → AST Construction → Code Generation.

---

## Module: AST (Abstract Syntax Tree)

**Header:** `src/ast.h`  
**Implementation:** `src/ast.cpp`  
**Purpose:** Core AST node management and type system

### Core Data Structures

#### ASTNode
The central data structure representing all C language constructs.

```c
typedef struct ASTNode {
    ASTNodeType type;           // Node type discriminator
    union { ... } data;         // Type-specific data
    TypeInfo* data_type;        // Optional type information
    ASTNode* next;             // Sibling node (for lists)
    int line;                  // Source line number
    int column;                // Source column number
} ASTNode;
```

#### ASTNodeType Enumeration
```c
typedef enum {
    // Expressions
    AST_IDENTIFIER,         // Variable/function names
    AST_CONSTANT,          // Numeric/character literals  
    AST_STRING_LITERAL,    // String constants
    AST_BINARY_OP,         // Binary operations (+, -, *, etc.)
    AST_UNARY_OP,          // Unary operations (!, -, ++, etc.)
    AST_ASSIGNMENT,        // Assignment operations
    AST_FUNCTION_CALL,     // Function invocations
    AST_ARRAY_ACCESS,      // Array element access
    AST_MEMBER_ACCESS,     // Struct member access
    AST_CAST,              // Type casting
    AST_CONDITIONAL,       // Ternary operator (?:)

    // Statements  
    AST_COMPOUND_STMT,     // Block statements { }
    AST_EXPRESSION_STMT,   // Expression statements
    AST_IF_STMT,           // Conditional statements
    AST_WHILE_STMT,        // While loops
    AST_FOR_STMT,          // For loops
    AST_DO_WHILE_STMT,     // Do-while loops
    AST_SWITCH_STMT,       // Switch statements
    AST_CASE_STMT,         // Case labels
    AST_DEFAULT_STMT,      // Default labels
    AST_BREAK_STMT,        // Break statements
    AST_CONTINUE_STMT,     // Continue statements
    AST_RETURN_STMT,       // Return statements
    AST_GOTO_STMT,         // Goto statements
    AST_LABEL_STMT,        // Label statements

    // Declarations
    AST_VARIABLE_DECL,     // Variable declarations
    AST_FUNCTION_DECL,     // Function declarations
    AST_FUNCTION_DEF,      // Function definitions
    AST_PARAMETER_DECL,    // Parameter declarations
    AST_STRUCT_DECL,       // Struct declarations
    AST_UNION_DECL,        // Union declarations
    AST_ENUM_DECL,         // Enum declarations
    AST_TYPEDEF_DECL       // Typedef declarations
} ASTNodeType;
```

### AST Node Creation Functions

#### `ASTNode* create_ast_node(ASTNodeType type)`
Creates a new AST node of the specified type.

**Parameters:**
- `type`: The type of AST node to create

**Returns:** 
- Pointer to newly allocated ASTNode
- NULL on allocation failure

**Example:**
```c
ASTNode* node = create_ast_node(AST_IDENTIFIER);
```

#### `ASTNode* create_identifier_node(char* name)`
Creates an identifier node with the given name.

**Parameters:**
- `name`: The identifier name (will be duplicated)

**Returns:**
- Pointer to identifier node
- NULL on allocation failure

**Example:**
```c
ASTNode* var = create_identifier_node("my_variable");
```

#### `ASTNode* create_constant_node(int value, TypeSpecifier type)`
Creates a constant node with the specified value and type.

**Parameters:**
- `value`: The constant value
- `type`: The type of the constant (TYPE_INT, TYPE_FLOAT, etc.)

**Returns:**
- Pointer to constant node  
- NULL on allocation failure

**Example:**
```c
ASTNode* num = create_constant_node(42, TYPE_INT);
```

#### `ASTNode* create_string_literal_node(char* string)`
Creates a string literal node.

**Parameters:**
- `string`: The string content (will be duplicated)

**Returns:**
- Pointer to string literal node
- NULL on allocation failure

**Example:**
```c
ASTNode* str = create_string_literal_node("Hello, World!");
```

#### `ASTNode* create_binary_op_node(BinaryOp op, ASTNode* left, ASTNode* right)`
Creates a binary operation node.

**Parameters:**
- `op`: The binary operator (OP_ADD, OP_SUB, etc.)
- `left`: Left operand AST node
- `right`: Right operand AST node

**Returns:**
- Pointer to binary operation node
- NULL on allocation failure

**Example:**
```c
ASTNode* left = create_constant_node(5, TYPE_INT);
ASTNode* right = create_constant_node(3, TYPE_INT);
ASTNode* add = create_binary_op_node(OP_ADD, left, right);
```

### Type System

#### TypeInfo Structure
Represents type information for variables, functions, and expressions.

```c
typedef struct TypeInfo {
    TypeSpecifier base_type;    // Base type (int, float, etc.)
    TypeQualifier qualifiers;   // const, volatile, etc.
    StorageClass storage_class; // static, extern, etc.
    int pointer_level;          // Number of pointer indirections
    TypeInfo* return_type;      // For functions/pointers
    TypeInfo* parameters;       // For function types
    TypeInfo* next;            // For parameter lists
    char* struct_name;         // For struct/union types
    int array_size;            // For array types
} TypeInfo;
```

#### `TypeInfo* create_type_info(TypeSpecifier type)`
Creates a new type information structure.

**Parameters:**
- `type`: Base type specifier

**Returns:**
- Pointer to TypeInfo structure
- NULL on allocation failure

#### `TypeInfo* create_pointer_type(TypeInfo* base_type)`
Creates a pointer type from a base type.

**Parameters:**
- `base_type`: The type being pointed to

**Returns:**
- Pointer type information
- NULL on allocation failure

#### `TypeInfo* create_array_type(TypeInfo* base_type, int size)`
Creates an array type.

**Parameters:**
- `base_type`: Element type
- `size`: Array size

**Returns:**
- Array type information
- NULL on allocation failure

### Memory Management

#### `void free_ast_node(ASTNode* node)`
Recursively frees an AST node and all its children.

**Parameters:**
- `node`: AST node to free (can be NULL)

**Note:** Automatically handles all node types and their specific data.

#### `void free_type_info(TypeInfo* type)`
Frees a TypeInfo structure and its linked components.

**Parameters:**
- `type`: TypeInfo to free (can be NULL)

### Symbol Table

#### Symbol Structure
```c
typedef struct Symbol {
    char* name;                // Symbol name
    TypeInfo* type;           // Symbol type
    bool is_global;           // Global scope flag
    bool is_parameter;        // Parameter flag
    int offset;               // Stack offset (for locals)
    struct Symbol* next;      // Next symbol in scope
} Symbol;
```

#### `Symbol* create_symbol(char* name, TypeInfo* type)`
Creates a new symbol table entry.

**Parameters:**
- `name`: Symbol name (will be duplicated)
- `type`: Symbol type information

**Returns:**
- Pointer to Symbol structure
- NULL on allocation failure

---

## Module: Code Generation

**Header:** `src/codegen.h`  
**Implementation:** `src/codegen.cpp`  
**Purpose:** LLVM IR generation from AST

### Core Structures

#### CodeGenContext
Central context for code generation state.

```c
typedef struct CodeGenContext {
    FILE* output;                    // Output file
    int next_reg_id;                // Next register ID
    int next_bb_id;                 // Next basic block ID
    Symbol* symbol_table;           // Current symbol table
    Symbol* global_symbols;         // Global symbol table
    BasicBlock* bb_list;            // Basic block list
    char* current_function_name;    // Current function context
} CodeGenContext;
```

### Code Generation Functions

#### `CodeGenContext* create_codegen_context(FILE* output)`
Creates a new code generation context.

**Parameters:**
- `output`: Output file for LLVM IR

**Returns:**
- Pointer to CodeGenContext
- NULL on allocation failure

#### `void generate_llvm_ir(CodeGenContext* ctx, ASTNode* ast)`
Main entry point for LLVM IR generation.

**Parameters:**
- `ctx`: Code generation context
- `ast`: Root AST node

**Note:** Generates complete LLVM IR module to the output file.

#### `LLVMValue* generate_expression(CodeGenContext* ctx, ASTNode* expr)`
Generates LLVM IR for expressions.

**Parameters:**
- `ctx`: Code generation context
- `expr`: Expression AST node

**Returns:**
- LLVMValue representing the expression result
- NULL on error

#### `void generate_statement(CodeGenContext* ctx, ASTNode* stmt)`
Generates LLVM IR for statements.

**Parameters:**
- `ctx`: Code generation context
- `stmt`: Statement AST node

### Utility Functions

#### `char* get_next_register(CodeGenContext* ctx)`
Allocates the next available register ID.

**Parameters:**
- `ctx`: Code generation context

**Returns:**
- String representation of register ID
- Caller must free the returned string

#### `void emit_instruction(CodeGenContext* ctx, const char* format, ...)`
Emits an LLVM IR instruction to the output.

**Parameters:**
- `ctx`: Code generation context
- `format`: Printf-style format string
- `...`: Format arguments

#### `char* llvm_type_to_string(TypeInfo* type)`
Converts TypeInfo to LLVM type string.

**Parameters:**
- `type`: Type information

**Returns:**
- LLVM type string (caller must free)
- "void" for NULL input

---

## Module: Error Handling

**Header:** `src/error_handling.h`  
**Implementation:** `src/error_handling.cpp`  
**Purpose:** Comprehensive error reporting and management

### Error Types

```c
typedef enum {
    ERROR_LEXICAL,      // Lexical analysis errors
    ERROR_SYNTAX,       // Syntax errors
    ERROR_SEMANTIC,     // Semantic analysis errors
    ERROR_CODEGEN,      // Code generation errors
    ERROR_PARSE,        // Parse errors
    ERROR_MEMORY,       // Memory allocation errors
    ERROR_IO,           // Input/output errors
    ERROR_INTERNAL      // Internal compiler errors
} ErrorType;
```

### Error Context

```c
typedef struct ErrorContext {
    ErrorType type;         // Error type
    char* file;            // Source file name
    int line;              // Line number
    char* function;        // Function name
    char* message;         // Error message
    struct ErrorContext* next; // Next error (for multiple errors)
} ErrorContext;
```

### Error Functions

#### `ErrorContext* create_error(ErrorType type, const char* file, int line, const char* function, const char* message)`
Creates a new error context.

**Parameters:**
- `type`: Type of error
- `file`: Source file name
- `line`: Line number
- `function`: Function name
- `message`: Error description

**Returns:**
- Pointer to ErrorContext
- NULL on allocation failure

#### `void report_error(ErrorContext* error)`
Reports an error to stderr with formatting.

**Parameters:**
- `error`: Error context to report

#### `const char* error_type_to_string(ErrorType type)`
Converts error type to human-readable string.

**Parameters:**
- `type`: Error type

**Returns:**
- String representation of error type

---

## Module: Memory Management

**Header:** `src/memory_management.h`  
**Implementation:** `src/memory_management.cpp`  
**Purpose:** Safe memory allocation and debugging

### Memory Statistics

```c
typedef struct MemoryStats {
    size_t allocations;        // Total allocations
    size_t deallocations;      // Total deallocations
    size_t bytes_allocated;    // Total bytes allocated
    size_t bytes_freed;        // Total bytes freed
    size_t peak_usage;         // Peak memory usage
    size_t current_usage;      // Current memory usage
} MemoryStats;
```

### Memory Context

```c
typedef struct MemoryContext {
    MemoryStats stats;         // Memory statistics
    bool debug_mode;          // Debug mode flag
    struct AllocationInfo* allocations; // Active allocations (debug)
} MemoryContext;
```

### Memory Functions

#### `void* safe_malloc_debug(size_t size, const char* file, int line, const char* function)`
Debug-aware memory allocation.

**Parameters:**
- `size`: Number of bytes to allocate
- `file`: Source file name
- `line`: Line number  
- `function`: Function name

**Returns:**
- Pointer to allocated memory
- Program exits on allocation failure

#### `void safe_free_debug(void* ptr, const char* file, int line, const char* function)`
Debug-aware memory deallocation.

**Parameters:**
- `ptr`: Pointer to free
- `file`: Source file name
- `line`: Line number
- `function`: Function name

#### `void* safe_calloc_debug(size_t count, size_t size, const char* file, int line, const char* function)`
Debug-aware zero-initialized allocation.

**Parameters:**
- `count`: Number of elements
- `size`: Size of each element
- `file`: Source file name
- `line`: Line number
- `function`: Function name

**Returns:**
- Pointer to zero-initialized memory
- Program exits on allocation failure

### Convenience Macros

```c
#define safe_malloc(size) safe_malloc_debug(size, __FILE__, __LINE__, __func__)
#define safe_free(ptr) safe_free_debug(ptr, __FILE__, __LINE__, __func__)
#define safe_calloc(count, size) safe_calloc_debug(count, size, __FILE__, __LINE__, __func__)
```

---

## Binary Operators

```c
typedef enum {
    // Arithmetic operators
    OP_ADD,             // +
    OP_SUB,             // -
    OP_MUL,             // *
    OP_DIV,             // /
    OP_MOD,             // %

    // Comparison operators  
    OP_LT,              // <
    OP_GT,              // >
    OP_LE,              // <=
    OP_GE,              // >=
    OP_EQ,              // ==
    OP_NE,              // !=

    // Logical operators
    OP_LOGICAL_AND,     // &&
    OP_LOGICAL_OR,      // ||

    // Bitwise operators
    OP_BITAND,          // &
    OP_BITOR,           // |
    OP_XOR,             // ^
    OP_LSHIFT,          // <<
    OP_RSHIFT,          // >>

    // Assignment operators
    OP_ASSIGN,          // =
    OP_ADD_ASSIGN,      // +=
    OP_SUB_ASSIGN,      // -=
    OP_MUL_ASSIGN,      // *=
    OP_DIV_ASSIGN,      // /=
    OP_MOD_ASSIGN,      // %=
    OP_AND_ASSIGN,      // &=
    OP_OR_ASSIGN,       // |=
    OP_XOR_ASSIGN,      // ^=
    OP_LSHIFT_ASSIGN,   // <<=
    OP_RSHIFT_ASSIGN    // >>=
} BinaryOp;
```

---

## Usage Examples

### Basic AST Construction
```c
// Create: x + 5
ASTNode* var = create_identifier_node("x");
ASTNode* num = create_constant_node(5, TYPE_INT);
ASTNode* expr = create_binary_op_node(OP_ADD, var, num);
```

### Type System Usage
```c
// Create: int* ptr
TypeInfo* int_type = create_type_info(TYPE_INT);
TypeInfo* ptr_type = create_pointer_type(int_type);
```

### Code Generation
```c
FILE* output = fopen("output.ll", "w");
CodeGenContext* ctx = create_codegen_context(output);
generate_llvm_ir(ctx, ast_root);
free_codegen_context(ctx);
fclose(output);
```

### Error Handling
```c
ErrorContext* error = create_error(ERROR_SEMANTIC, "test.c", 42, 
                                  "parse_function", "Undefined variable");
report_error(error);
free_error(error);
```

---

## Best Practices

### Memory Management
1. Always check return values from allocation functions
2. Use the safe_* macros for automatic debugging
3. Free AST nodes with `free_ast_node()` for proper cleanup
4. Pair every `create_*` call with appropriate `free_*` call

### Error Handling  
1. Create specific error contexts for better debugging
2. Include source location information when available
3. Use appropriate error types for categorization
4. Report errors early and fail fast

### AST Construction
1. Build AST bottom-up (leaves first, then parents)
2. Set type information when known
3. Maintain parent-child relationships correctly
4. Use NULL checks before accessing node data

### Code Generation
1. Initialize CodeGenContext before generation
2. Generate expressions before using their results
3. Emit proper LLVM IR syntax
4. Handle all AST node types in generation functions

---

*This API documentation covers all public interfaces and core functionality of the C-to-LLVM IR compiler. For implementation details, refer to the source code and inline comments.*
typedef unsigned long size_t;
typedef void* FILE;
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long int64_t;
typedef unsigned long uint64_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
typedef _Bool bool;
typedef int va_list;
int printf(const char* format, ...);
int fprintf(FILE* stream, const char* format, ...);
int sprintf(char* str, const char* format, ...);
FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
int fputs(const char* s, FILE* stream);
int fflush(FILE* stream);
void* malloc(unsigned long size);
void* realloc(void* ptr, unsigned long size);
void free(void* ptr);
void* memset(void* s, int c, unsigned long n);
void* memcpy(void* dest, const void* src, unsigned long n);
int strcmp(const char* s1, const char* s2);
int atoi(const char* s);
long strtol(const char* s, char** endptr, int base);
unsigned long strlen(const char* s);
char* strdup(const char* s);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
char* strstr(const char* haystack, const char* needle);
void exit(int status);
typedef struct Arena Arena;
Arena* arena_create(size_t initial_capacity);
void* arena_alloc(Arena* arena, size_t size);
void arena_reset(Arena* arena);
void arena_destroy(Arena* arena);
extern Arena* g_compiler_arena;
void mem_init(void);
void mem_cleanup(void);
char* arena_strdup(Arena* arena, const char* str);
void error_report(const char* format, ...);
void fatal_error(const char* format, ...);
int error_get_count(void);
void error_suppress_output(bool suppress);
typedef struct ASTNode ASTNode;
typedef struct TypeInfo TypeInfo;
typedef struct Symbol Symbol;
typedef enum {
    AST_IDENTIFIER,
    AST_CONSTANT,
    AST_STRING_LITERAL,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_ASSIGNMENT,
    AST_FUNCTION_CALL,
    AST_ARRAY_ACCESS,
    AST_MEMBER_ACCESS,
    AST_CAST,
    AST_CONDITIONAL,
    AST_COMPOUND_STMT,
    AST_EXPRESSION_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_DO_WHILE_STMT,
    AST_SWITCH_STMT,
    AST_CASE_STMT,
    AST_DEFAULT_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_RETURN_STMT,
    AST_GOTO_STMT,
    AST_LABEL_STMT,
    AST_VARIABLE_DECL,
    AST_FUNCTION_DECL,
    AST_FUNCTION_DEF,
    AST_PARAMETER_DECL,
    AST_STRUCT_DECL,
    AST_UNION_DECL,
    AST_ENUM_DECL,
    AST_TYPEDEF_DECL,
    AST_POINTER_TYPE,
    AST_ARRAY_TYPE,
    AST_FUNCTION_TYPE,
    AST_STRUCT_TYPE,
    AST_UNION_TYPE,
    AST_ENUM_TYPE,
    AST_BASIC_TYPE,
    AST_TRANSLATION_UNIT,
    AST_INITIALIZER_LIST,
    AST_ARGUMENT_LIST,
    AST_PARAMETER_LIST,
    AST_DECLARATION_LIST,
    AST_STATEMENT_LIST
} ASTNodeType;
typedef enum {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_SHORT,
    TYPE_INT,
    TYPE_LONG,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_SIGNED,
    TYPE_UNSIGNED,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_ENUM,
    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_FUNCTION
} DataType;
typedef enum { QUAL_NONE = 0, QUAL_CONST = 1, QUAL_VOLATILE = 2 } TypeQualifier;
typedef enum {
    STORAGE_NONE,
    STORAGE_AUTO,
    STORAGE_REGISTER,
    STORAGE_STATIC,
    STORAGE_EXTERN,
    STORAGE_TYPEDEF
} StorageClass;
typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_LT, OP_GT, OP_LE, OP_GE, OP_EQ, OP_NE,
    OP_AND, OP_OR, OP_BITAND, OP_BITOR, OP_XOR,
    OP_LSHIFT, OP_RSHIFT,
    OP_ASSIGN, OP_ADD_ASSIGN, OP_SUB_ASSIGN,
    OP_MUL_ASSIGN, OP_DIV_ASSIGN, OP_MOD_ASSIGN,
    OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN,
    OP_LSHIFT_ASSIGN, OP_RSHIFT_ASSIGN,
    OP_COMMA
} BinaryOp;
typedef enum {
    UOP_PLUS, UOP_MINUS, UOP_NOT, UOP_BITNOT,
    UOP_PREINC, UOP_PREDEC, UOP_POSTINC, UOP_POSTDEC,
    UOP_ADDR, UOP_DEREF, UOP_SIZEOF
} UnaryOp;
struct TypeInfo {
    DataType base_type;
    TypeQualifier qualifiers;
    StorageClass storage_class;
    int pointer_level;
    int array_size;
    struct TypeInfo* return_type;
    struct ASTNode* parameters;
    char* struct_name;
    struct Symbol* struct_members;
    int size;
    int alignment;
    struct TypeInfo* next;
};
struct Symbol {
    char* name;
    TypeInfo* type;
    int offset;
    int index;
    int is_global;
    int is_parameter;
    int is_array;
    int is_enum_constant;
    int enum_value;
    struct Symbol* next;
};
struct ASTNode {
    ASTNodeType type;
    TypeInfo* data_type;
    union {
        struct {
            char* name;
            Symbol* symbol;
            ASTNode* parameters;
            int is_variadic;
            int pointer_level;
            int is_function_pointer;
            struct ASTNode* array_dimensions;
        } identifier;
        struct {
            union {
                int int_val;
                float float_val;
                char char_val;
            } value;
            DataType const_type;
        } constant;
        struct {
            char* string;
            int length;
        } string_literal;
        struct {
            BinaryOp op;
            ASTNode* left;
            ASTNode* right;
        } binary_op;
        struct {
            UnaryOp op;
            ASTNode* operand;
        } unary_op;
        struct {
            ASTNode* function;
            ASTNode* arguments;
        } function_call;
        struct {
            ASTNode* array;
            ASTNode* index;
        } array_access;
        struct {
            ASTNode* object;
            char* member;
            int is_pointer_access;
        } member_access;
        struct {
            TypeInfo* target_type;
            ASTNode* operand;
        } cast_expr;
        struct {
            ASTNode* condition;
            ASTNode* then_expr;
            ASTNode* else_expr;
        } conditional_expr;
        struct {
            ASTNode* statements;
            int num_statements;
        } compound_stmt;
        struct {
            ASTNode* condition;
            ASTNode* then_stmt;
            ASTNode* else_stmt;
        } if_stmt;
        struct {
            ASTNode* condition;
            ASTNode* body;
        } while_stmt;
        struct {
            ASTNode* init;
            ASTNode* condition;
            ASTNode* update;
            ASTNode* body;
        } for_stmt;
        struct {
            ASTNode* expression;
        } return_stmt;
        struct {
            ASTNode* expression;
            ASTNode* body;
        } switch_stmt;
        struct {
            ASTNode* value;
            ASTNode* statement;
        } case_stmt;
        struct {
            TypeInfo* type;
            char* name;
            ASTNode* initializer;
            ASTNode* parameters;
            int pointer_level;
            struct ASTNode* array_dimensions;
        } variable_decl;
        struct {
            TypeInfo* return_type;
            char* name;
            ASTNode* parameters;
            ASTNode* body;
            int is_variadic;
            int pointer_level;
        } function_def;
        struct {
            char* name;
            ASTNode* members;
            Symbol* symbol_table;
        } struct_decl;
        struct {
            ASTNode* items;
            int count;
        } initializer_list;
        struct {
            ASTNode** items;
            int count;
            int capacity;
        } list;
    } data;
    int line;
    int column;
    ASTNode* next;
};
ASTNode* create_ast_node(ASTNodeType type);
ASTNode* create_identifier_node(const char* name);
ASTNode* create_constant_node(int value, DataType type);
ASTNode* create_string_literal_node(const char* string);
int parse_constant_value(const char* s);
int evaluate_constant_node(ASTNode* node);
ASTNode* create_binary_op_node(BinaryOp op, ASTNode* left, ASTNode* right);
ASTNode* create_unary_op_node(UnaryOp op, ASTNode* operand);
ASTNode* create_function_call_node(ASTNode* function, ASTNode* arguments);
ASTNode* create_compound_stmt_node(ASTNode* statements);
ASTNode* create_if_stmt_node(ASTNode* condition, ASTNode* then_stmt, ASTNode* else_stmt);
ASTNode* create_while_stmt_node(ASTNode* condition, ASTNode* body);
ASTNode* create_for_stmt_node(ASTNode* init, ASTNode* condition, ASTNode* update, ASTNode* body);
ASTNode* create_return_stmt_node(ASTNode* expression);
ASTNode* create_variable_decl_node(TypeInfo* type, const char* name, ASTNode* initializer);
ASTNode* create_function_decl_node(TypeInfo* return_type, const char* name, ASTNode* parameters, int is_variadic);
ASTNode* create_function_def_node(TypeInfo* return_type, const char* name, ASTNode* parameters, ASTNode* body, int is_variadic);
TypeInfo* create_type_info(DataType base_type);
int get_type_size(TypeInfo* type);
int get_type_alignment(TypeInfo* type);
TypeInfo* create_struct_type(const char* tag, int is_union);
void struct_add_member(TypeInfo* type, const char* name, TypeInfo* member_type);
void struct_finish_layout(TypeInfo* type);
Symbol* struct_lookup_member(TypeInfo* type, const char* name);
TypeInfo* duplicate_type_info(TypeInfo* original);
TypeInfo* create_pointer_type(TypeInfo* base_type);
TypeInfo* create_array_type(TypeInfo* base_type, int size);
TypeInfo* create_function_type(TypeInfo* return_type, ASTNode* parameters);
void free_ast_node(ASTNode* node);
void free_type_info(TypeInfo* type);
void print_ast(ASTNode* node, int indent);
void print_type_info(const TypeInfo* type);
Symbol* create_symbol(const char* name, TypeInfo* type);
void free_symbol(Symbol* symbol);
void symbol_add_global(Symbol* symbol);
void symbol_add_local(Symbol* symbol);
Symbol* symbol_lookup(const char* name);
void tag_add(Symbol* symbol);
Symbol* tag_lookup(const char* name);
void symbol_clear_locals(void);
void symbol_clear_all(void);
void symbol_init_builtins(void);
extern Symbol* g_global_symbols;
extern Symbol* g_local_symbols;
extern Symbol* g_tags;
typedef struct LLVMValue LLVMValue;
typedef enum {
    LLVM_VALUE_REGISTER,
    LLVM_VALUE_GLOBAL,
    LLVM_VALUE_CONSTANT,
    LLVM_VALUE_FUNCTION
} LLVMValueType;
struct LLVMValue {
    LLVMValueType type;
    char* name;
    TypeInfo* llvm_type;
    int is_lvalue;
    union {
        int constant_val;
    } data;
};
typedef struct {
    FILE* output;
    int next_reg_id;
    int next_bb_id;
    char* current_function_name;
    TypeInfo* current_function_return_type;
} CodeGenContext;
void codegen_init(FILE* output);
void codegen_run(ASTNode* ast);
void codegen_cleanup(void);
LLVMValue* gen_expression(ASTNode* expr);
void gen_statement(ASTNode* stmt);

extern int yyparse(void);
extern FILE* yyin;
extern ASTNode* program_ast;
int main(int argc, char* argv[]) {
    mem_init();
    symbol_init_builtins();
    codegen_init(stdout);
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            fatal_error("Cannot open input file: %s", argv[1]);
        }
    } else {
        yyin = stdin;
    }
    if (yyparse() == 0 && program_ast) {
        codegen_run(program_ast);
    } else {
        error_report("Compilation failed due to errors.");
    }
    if (yyin != stdin) fclose(yyin);
    mem_cleanup();
    return error_get_count() > 0 ? 1 : 0;
}

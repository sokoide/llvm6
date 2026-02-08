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

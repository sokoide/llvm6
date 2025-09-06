#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
typedef struct ASTNode ASTNode;
typedef struct TypeInfo TypeInfo;
typedef struct Symbol Symbol;

/* AST Node Types */
typedef enum {
    /* Expressions */
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
    
    /* Statements */
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
    
    /* Declarations */
    AST_VARIABLE_DECL,
    AST_FUNCTION_DECL,
    AST_FUNCTION_DEF,
    AST_PARAMETER_DECL,
    AST_STRUCT_DECL,
    AST_UNION_DECL,
    AST_ENUM_DECL,
    AST_TYPEDEF_DECL,
    
    /* Types */
    AST_POINTER_TYPE,
    AST_ARRAY_TYPE,
    AST_FUNCTION_TYPE,
    AST_STRUCT_TYPE,
    AST_UNION_TYPE,
    AST_ENUM_TYPE,
    AST_BASIC_TYPE,
    
    /* Others */
    AST_TRANSLATION_UNIT,
    AST_INITIALIZER_LIST,
    AST_ARGUMENT_LIST,
    AST_PARAMETER_LIST,
    AST_DECLARATION_LIST,
    AST_STATEMENT_LIST
} ASTNodeType;

/* Data Types */
typedef enum {
    TYPE_VOID,
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

/* Type qualifiers */
typedef enum {
    QUAL_NONE = 0,
    QUAL_CONST = 1,
    QUAL_VOLATILE = 2
} TypeQualifier;

/* Storage classes */
typedef enum {
    STORAGE_NONE,
    STORAGE_AUTO,
    STORAGE_REGISTER,
    STORAGE_STATIC,
    STORAGE_EXTERN,
    STORAGE_TYPEDEF
} StorageClass;

/* Binary operators */
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

/* Unary operators */
typedef enum {
    UOP_PLUS, UOP_MINUS, UOP_NOT, UOP_BITNOT,
    UOP_PREINC, UOP_PREDEC, UOP_POSTINC, UOP_POSTDEC,
    UOP_ADDR, UOP_DEREF, UOP_SIZEOF
} UnaryOp;

/* Type information */
struct TypeInfo {
    DataType base_type;
    TypeQualifier qualifiers;
    StorageClass storage_class;
    int pointer_level;
    int array_size;
    struct TypeInfo* return_type;  /* for function types */
    struct ASTNode* parameters;    /* for function types */
    char* struct_name;            /* for struct/union/enum types */
    struct TypeInfo* next;        /* for type lists */
};

/* Symbol table entry */
struct Symbol {
    char* name;
    TypeInfo* type;
    int offset;                   /* for local variables */
    int is_global;
    struct Symbol* next;
};

/* AST Node structure */
struct ASTNode {
    ASTNodeType type;
    TypeInfo* data_type;
    
    union {
        /* Terminals */
        struct {
            char* name;
            Symbol* symbol;
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
        
        /* Binary operations */
        struct {
            BinaryOp op;
            ASTNode* left;
            ASTNode* right;
        } binary_op;
        
        /* Unary operations */
        struct {
            UnaryOp op;
            ASTNode* operand;
        } unary_op;
        
        /* Function calls */
        struct {
            ASTNode* function;
            ASTNode* arguments;
        } function_call;
        
        /* Array access */
        struct {
            ASTNode* array;
            ASTNode* index;
        } array_access;
        
        /* Member access */
        struct {
            ASTNode* object;
            char* member;
            int is_pointer_access;  /* -> vs . */
        } member_access;
        
        /* Statements */
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
        
        /* Declarations */
        struct {
            TypeInfo* type;
            char* name;
            ASTNode* initializer;
        } variable_decl;
        
        struct {
            TypeInfo* return_type;
            char* name;
            ASTNode* parameters;
            ASTNode* body;
        } function_def;
        
        /* Lists */
        struct {
            ASTNode** items;
            int count;
            int capacity;
        } list;
    } data;
    
    /* Source location info */
    int line;
    int column;
    
    /* Next sibling for lists */
    ASTNode* next;
};

/* Function prototypes */
ASTNode* create_ast_node(ASTNodeType type);
ASTNode* create_identifier_node(char* name);
ASTNode* create_constant_node(int value, DataType type);
ASTNode* create_string_literal_node(char* string);
ASTNode* create_binary_op_node(BinaryOp op, ASTNode* left, ASTNode* right);
ASTNode* create_unary_op_node(UnaryOp op, ASTNode* operand);
ASTNode* create_function_call_node(ASTNode* function, ASTNode* arguments);
ASTNode* create_compound_stmt_node(ASTNode* statements);
ASTNode* create_if_stmt_node(ASTNode* condition, ASTNode* then_stmt, ASTNode* else_stmt);
ASTNode* create_while_stmt_node(ASTNode* condition, ASTNode* body);
ASTNode* create_for_stmt_node(ASTNode* init, ASTNode* condition, ASTNode* update, ASTNode* body);
ASTNode* create_return_stmt_node(ASTNode* expression);
ASTNode* create_variable_decl_node(TypeInfo* type, char* name, ASTNode* initializer);
ASTNode* create_function_def_node(TypeInfo* return_type, char* name, ASTNode* parameters, ASTNode* body);

TypeInfo* create_type_info(DataType base_type);
TypeInfo* create_pointer_type(TypeInfo* base_type);
TypeInfo* create_array_type(TypeInfo* base_type, int size);
TypeInfo* create_function_type(TypeInfo* return_type, ASTNode* parameters);

void free_ast_node(ASTNode* node);
void free_type_info(TypeInfo* type);

void print_ast(ASTNode* node, int indent);
void print_type_info(TypeInfo* type);

/* Symbol table functions */
Symbol* create_symbol(char* name, TypeInfo* type);
void free_symbol(Symbol* symbol);

#endif /* AST_H */
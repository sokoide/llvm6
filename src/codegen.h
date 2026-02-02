#ifndef CODEGEN_H
#define CODEGEN_H

extern "C" {

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
typedef struct CodeGenContext CodeGenContext;
typedef struct LLVMValue LLVMValue;
typedef struct BasicBlock BasicBlock;

/* LLVM value representation */
typedef enum {
    LLVM_VALUE_REGISTER,
    LLVM_VALUE_GLOBAL,
    LLVM_VALUE_CONSTANT,
    LLVM_VALUE_FUNCTION,
    LLVM_VALUE_BASIC_BLOCK
} LLVMValueType;

struct LLVMValue {
    LLVMValueType type;
    char* name;
    TypeInfo* llvm_type;
    union {
        int reg_id;
        int constant_val;
        char* global_name;
    } data;
};

/* Basic block for control flow */
struct BasicBlock {
    char* label;
    int id;
    BasicBlock* next;
};

/* Code generation context */
struct CodeGenContext {
    FILE* output;
    int next_reg_id;
    int next_bb_id;
    int current_function_id;

    /* Symbol tables */
    Symbol* global_symbols;
    Symbol* local_symbols;

    /* Control flow */
    BasicBlock* current_bb;
    BasicBlock* bb_list;

    /* Function information */
    char* current_function_name;
    TypeInfo* current_function_return_type;

    /* Temporary storage */
    char temp_buffer[1024];
    int indent_level;
};

/* Function prototypes */
CodeGenContext* create_codegen_context(FILE* output);
void free_codegen_context(CodeGenContext* ctx);

/* Main code generation functions */
void generate_llvm_ir(CodeGenContext* ctx, ASTNode* ast);
LLVMValue* generate_expression(CodeGenContext* ctx, ASTNode* expr);
void generate_statement(CodeGenContext* ctx, ASTNode* stmt);
void generate_declaration(CodeGenContext* ctx, ASTNode* decl);
void generate_function_definition(CodeGenContext* ctx, ASTNode* func_def);

/* Expression generation */
LLVMValue* generate_binary_op(CodeGenContext* ctx, ASTNode* expr);

LLVMValue* generate_assignment_op(CodeGenContext* ctx, ASTNode* expr);
LLVMValue* generate_unary_op(CodeGenContext* ctx, ASTNode* expr);
LLVMValue* generate_function_call(CodeGenContext* ctx, ASTNode* call);
LLVMValue* generate_array_access(CodeGenContext* ctx, ASTNode* access);
LLVMValue* generate_member_access(CodeGenContext* ctx, ASTNode* access);
LLVMValue* generate_pointer_arithmetic_op(CodeGenContext* ctx, BinaryOp op,
                                          LLVMValue* left, LLVMValue* right);
LLVMValue* generate_identifier(CodeGenContext* ctx, ASTNode* identifier);
LLVMValue* generate_constant(CodeGenContext* ctx, ASTNode* constant);
LLVMValue* generate_string_literal(CodeGenContext* ctx, ASTNode* string_lit);

/* Statement generation */
void generate_compound_statement(CodeGenContext* ctx, ASTNode* stmt);
void generate_if_statement(CodeGenContext* ctx, ASTNode* stmt);
void generate_while_statement(CodeGenContext* ctx, ASTNode* stmt);
void generate_for_statement(CodeGenContext* ctx, ASTNode* stmt);
void generate_return_statement(CodeGenContext* ctx, ASTNode* stmt);
void generate_expression_statement(CodeGenContext* ctx, ASTNode* stmt);

/* Type conversion */
char* llvm_type_to_string(TypeInfo* type);
char* get_default_value(const TypeInfo* type);
int get_type_size(TypeInfo* type);
int types_compatible(TypeInfo* type1, TypeInfo* type2);

/* Utility functions */
LLVMValue* create_llvm_value(LLVMValueType type, const char* name,
                             TypeInfo* llvm_type);
void free_llvm_value(LLVMValue* value);

char* get_next_register(CodeGenContext* ctx);
char* get_next_basic_block(CodeGenContext* ctx);
BasicBlock* create_basic_block(CodeGenContext* ctx, const char* label);

/* Symbol table management */
void add_global_symbol(CodeGenContext* ctx, Symbol* symbol);
void add_local_symbol(CodeGenContext* ctx, Symbol* symbol);
Symbol* lookup_symbol(CodeGenContext* ctx, const char* name);
void clear_local_symbols(CodeGenContext* ctx);

/* Output functions */
void emit_instruction(CodeGenContext* ctx, const char* format, ...);
void emit_global_declaration(CodeGenContext* ctx, const char* format, ...);
void emit_function_header(CodeGenContext* ctx, const char* format, ...);
void emit_basic_block_label(CodeGenContext* ctx, const char* label);
void emit_comment(CodeGenContext* ctx, const char* comment);

/* Built-in functions and runtime support */
void generate_runtime_declarations(CodeGenContext* ctx);
void generate_main_function_wrapper(CodeGenContext* ctx);

/* Debugging and error reporting */
void codegen_error(CodeGenContext* ctx, const char* message, ...);
void codegen_warning(CodeGenContext* ctx, const char* message, ...);
}
#endif /* CODEGEN_H */
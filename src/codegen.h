#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "symbols.h"

/* Forward declarations */
typedef struct LLVMValue LLVMValue;

/* LLVM value representation */
typedef enum {
    LLVM_VALUE_REGISTER,
    LLVM_VALUE_GLOBAL,
    LLVM_VALUE_CONSTANT,
    LLVM_VALUE_FUNCTION
} LLVMValueType;

struct LLVMValue {
    LLVMValueType type;
    char* name; /* Register name or constant string */
    TypeInfo* llvm_type;
    int is_lvalue;
    union {
        int constant_val;
    } data;
};

/* Code generation context */
typedef struct {
    FILE* output;
    int next_reg_id;
    int next_bb_id;
    char* current_function_name;
    TypeInfo* current_function_return_type;
} CodeGenContext;

/* Function prototypes */
void codegen_init(FILE* output);
void codegen_run(ASTNode* ast);
void codegen_cleanup(void);

/* Internal generators */
LLVMValue* gen_expression(ASTNode* expr);
void gen_statement(ASTNode* stmt);

#endif /* CODEGEN_H */
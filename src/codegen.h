#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "symbols.h"

/* Forward declarations */
typedef struct CodeGenContext CodeGenContext;
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
    char* name;
    TypeInfo* llvm_type;
    int is_lvalue;
    union {
        int constant_val;
    } data;
};

struct CodeGenContext {
    FILE* output;
    int next_reg_id;
    int next_bb_id;
    char* current_function_name;
    TypeInfo* current_function_return_type;
    char* loop_break_label;
    char* loop_continue_label;
};

/* Function prototypes */
void codegen_init(FILE* output);
void codegen_run(ASTNode* ast);
void codegen_cleanup(void);

#endif /* CODEGEN_H */

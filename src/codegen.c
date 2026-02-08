#include "codegen.h"

static CodeGenContext g_ctx;

void codegen_init(FILE* output) {
    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.output = output ? output : stdout;
    g_ctx.next_reg_id = 1;
    g_ctx.next_bb_id = 1;
}

char* llvm_type_to_string(TypeInfo* type) {
    if (!type) return arena_strdup(g_compiler_arena, "i32");

    switch (type->base_type) {
        case TYPE_VOID: return arena_strdup(g_compiler_arena, "void");
        case TYPE_CHAR: return arena_strdup(g_compiler_arena, "i8");
        case TYPE_SHORT: return arena_strdup(g_compiler_arena, "i16");
        case TYPE_INT: return arena_strdup(g_compiler_arena, "i32");
        case TYPE_LONG: return arena_strdup(g_compiler_arena, "i64");
        case TYPE_POINTER: {
            char* base = llvm_type_to_string(type->return_type);
            char* ptr = (char*)arena_alloc(g_compiler_arena, strlen(base) + 2);
            sprintf(ptr, "%s*", base);
            return ptr;
        }
        default: return arena_strdup(g_compiler_arena, "i32");
    }
}

void emit_instruction(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(g_ctx.output, "  ");
    vfprintf(g_ctx.output, format, args);
    fprintf(g_ctx.output, "\n");
    va_end(args);
}

void codegen_run(ASTNode* ast) {
    fprintf(g_ctx.output, "; Generated LLVM IR\n");
    fprintf(g_ctx.output, "target triple = \"arm64-apple-darwin\"\n\n");

    /* Register printf */
    fprintf(g_ctx.output, "declare i32 @printf(i8*, ...)\n\n");

    /* Basic traversal - simplified for now */
    ASTNode* curr = ast;
    while (curr) {
        if (curr->type == AST_FUNCTION_DEF) {
            char* ret_type = llvm_type_to_string(curr->data.function_def.return_type);
            fprintf(g_ctx.output, "define %s @%s() {\n", ret_type, curr->data.function_def.name);
            
            /* Body - simplified return */
            ASTNode* body = curr->data.function_def.body;
            if (body && body->type == AST_COMPOUND_STMT) {
                ASTNode* stmt = body->data.compound_stmt.statements;
                while (stmt) {
                    if (stmt->type == AST_RETURN_STMT) {
                        if (stmt->data.return_stmt.expression && stmt->data.return_stmt.expression->type == AST_CONSTANT) {
                            emit_instruction("ret i32 %d", stmt->data.return_stmt.expression->data.constant.value.int_val);
                        } else {
                            emit_instruction("ret void");
                        }
                    }
                    stmt = stmt->next;
                }
            }
            fprintf(g_ctx.output, "}\n\n");
        }
        curr = curr->next;
    }
}

void codegen_cleanup(void) {
    /* Context state is in arena, so just reset */
}
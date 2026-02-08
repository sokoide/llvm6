#include "codegen.h"

static CodeGenContext g_ctx;

void codegen_init(FILE* output) {
    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.output = output ? output : stdout;
    g_ctx.next_reg_id = 1;
    g_ctx.next_bb_id = 1;
}

char* get_next_reg(void) {
    char* reg = (char*)arena_alloc(g_compiler_arena, 16);
    sprintf(reg, "r%d", g_ctx.next_reg_id++);
    return reg;
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
            if (!type->return_type) return arena_strdup(g_compiler_arena, "i8*");
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

LLVMValue* create_llvm_value(LLVMValueType type, const char* name, TypeInfo* llvm_type) {
    LLVMValue* val = (LLVMValue*)arena_alloc(g_compiler_arena, sizeof(LLVMValue));
    val->type = type;
    val->name = arena_strdup(g_compiler_arena, name);
    val->llvm_type = llvm_type;
    return val;
}

LLVMValue* gen_expression(ASTNode* expr) {
    if (!expr) return NULL;

    switch (expr->type) {
        case AST_IDENTIFIER: {
            Symbol* sym = symbol_lookup(expr->data.identifier.name);
            if (!sym) {
                error_report("Undefined identifier: %s", expr->data.identifier.name);
                return NULL;
            }
            char* reg = get_next_reg();
            char* type_str = llvm_type_to_string(sym->type);
            emit_instruction("%%%s = load %s, %s* %%%s", reg, type_str, type_str, sym->name);
            return create_llvm_value(LLVM_VALUE_REGISTER, reg, sym->type);
        }
        case AST_CONSTANT: {
            char buf[32];
            sprintf(buf, "%d", expr->data.constant.value.int_val);
            LLVMValue* val = create_llvm_value(LLVM_VALUE_CONSTANT, buf, create_type_info(TYPE_INT));
            val->data.constant_val = expr->data.constant.value.int_val;
            return val;
        }
        case AST_BINARY_OP: {
            /* Handle regular binary ops */
            LLVMValue* left = gen_expression(expr->data.binary_op.left);
            LLVMValue* right = gen_expression(expr->data.binary_op.right);
            if (!left || !right) return NULL;

            const char* op_name = NULL;
            switch (expr->data.binary_op.op) {
                case OP_ADD: op_name = "add"; break;
                case OP_SUB: op_name = "sub"; break;
                case OP_MUL: op_name = "mul"; break;
                case OP_DIV: op_name = "sdiv"; break;
                case OP_MOD: op_name = "srem"; break;
                case OP_BITAND: op_name = "and"; break;
                case OP_BITOR:  op_name = "or";  break;
                case OP_XOR:    op_name = "xor"; break;
                case OP_LSHIFT: op_name = "shl"; break;
                case OP_RSHIFT: op_name = "ashr"; break;
                case OP_ASSIGN: {
                    /* Special case: Simple assignment */
                    ASTNode* target = expr->data.binary_op.left;
                    if (target->type != AST_IDENTIFIER) {
                        error_report("Assignment to non-variable");
                        return NULL;
                    }
                    Symbol* sym = symbol_lookup(target->data.identifier.name);
                    if (!sym) return NULL;
                    
                    char* type_str = llvm_type_to_string(sym->type);
                    emit_instruction("store %s %s%s, %s* %%%s", 
                        type_str, 
                        right->type == LLVM_VALUE_REGISTER ? "%" : "", right->name,
                        type_str, sym->name);
                    return right;
                }
                default: {
                    /* Handle Compound Assignments */
                    const char* base_op = NULL;
                    switch (expr->data.binary_op.op) {
                        case OP_ADD_ASSIGN: base_op = "add"; break;
                        case OP_SUB_ASSIGN: base_op = "sub"; break;
                        case OP_MUL_ASSIGN: base_op = "mul"; break;
                        case OP_DIV_ASSIGN: base_op = "sdiv"; break;
                        case OP_MOD_ASSIGN: base_op = "srem"; break;
                        case OP_AND_ASSIGN: base_op = "and"; break;
                        case OP_OR_ASSIGN:  base_op = "or";  break;
                        case OP_XOR_ASSIGN: base_op = "xor"; break;
                        case OP_LSHIFT_ASSIGN: base_op = "shl"; break;
                        case OP_RSHIFT_ASSIGN: base_op = "ashr"; break;
                        default: return NULL;
                    }
                    
                    ASTNode* target = expr->data.binary_op.left;
                    if (target->type != AST_IDENTIFIER) return NULL;
                    Symbol* sym = symbol_lookup(target->data.identifier.name);
                    if (!sym) return NULL;

                    char* type_str = llvm_type_to_string(sym->type);
                    char* res_reg = get_next_reg();
                    
                    /* Load, op, store */
                    emit_instruction("%%%s = %s %s %s%s, %s%s", 
                        res_reg, base_op, type_str,
                        left->type == LLVM_VALUE_REGISTER ? "%" : "", left->name,
                        right->type == LLVM_VALUE_REGISTER ? "%" : "", right->name);
                    
                    emit_instruction("store %s %%%s, %s* %%%s", 
                        type_str, res_reg, type_str, sym->name);
                    
                    return create_llvm_value(LLVM_VALUE_REGISTER, res_reg, sym->type);
                }
            }

            char* reg = get_next_reg();
            char* type_str = llvm_type_to_string(left->llvm_type);
            emit_instruction("%%%s = %s %s %s%s, %s%s", 
                reg, op_name, type_str, 
                left->type == LLVM_VALUE_REGISTER ? "%" : "", left->name,
                right->type == LLVM_VALUE_REGISTER ? "%" : "", right->name);
            
            return create_llvm_value(LLVM_VALUE_REGISTER, reg, left->llvm_type);
        }
        case AST_UNARY_OP: {
            LLVMValue* operand = gen_expression(expr->data.unary_op.operand);
            if (!operand) return NULL;

            if (expr->data.unary_op.op == UOP_BITNOT) {
                char* reg = get_next_reg();
                char* type_str = llvm_type_to_string(operand->llvm_type);
                emit_instruction("%%%s = xor %s %s%s, -1",
                    reg, type_str,
                    operand->type == LLVM_VALUE_REGISTER ? "%" : "", operand->name);
                return create_llvm_value(LLVM_VALUE_REGISTER, reg, operand->llvm_type);
            }
            return operand;
        }
        default:
            return NULL;
    }
}

void gen_statement(ASTNode* stmt) {
    if (!stmt) return;

    switch (stmt->type) {
        case AST_COMPOUND_STMT: {
            ASTNode* curr = stmt->data.compound_stmt.statements;
            while (curr) {
                gen_statement(curr);
                curr = curr->next;
            }
            break;
        }
        case AST_VARIABLE_DECL: {
            Symbol* sym = create_symbol(stmt->data.variable_decl.name, stmt->data.variable_decl.type);
            symbol_add_local(sym);
            
            char* type_str = llvm_type_to_string(sym->type);
            emit_instruction("%%%s = alloca %s", sym->name, type_str);
            
            if (stmt->data.variable_decl.initializer) {
                LLVMValue* init = gen_expression(stmt->data.variable_decl.initializer);
                if (init) {
                    emit_instruction("store %s %s%s, %s* %%%s", 
                        type_str, 
                        init->type == LLVM_VALUE_REGISTER ? "%" : "", init->name,
                        type_str, sym->name);
                }
            }
            break;
        }
        case AST_RETURN_STMT: {
            LLVMValue* val = gen_expression(stmt->data.return_stmt.expression);
            if (val) {
                char* type_str = llvm_type_to_string(val->llvm_type);
                emit_instruction("ret %s %s%s", type_str, 
                    val->type == LLVM_VALUE_REGISTER ? "%" : "", val->name);
            } else {
                emit_instruction("ret void");
            }
            break;
        }
        case AST_EXPRESSION_STMT: {
            gen_expression(stmt->data.return_stmt.expression);
            break;
        }
        default:
            break;
    }
}

void codegen_run(ASTNode* ast) {
    if (!ast) return;

    fprintf(g_ctx.output, "; Generated LLVM IR\n");
    fprintf(g_ctx.output, "target triple = \"arm64-apple-darwin\"\n\n");

    /* Register printf */
    fprintf(g_ctx.output, "declare i32 @printf(i8*, ...)\n\n");

    ASTNode* curr = ast;
    while (curr) {
        if (curr->type == AST_FUNCTION_DEF) {
            symbol_clear_locals();
            char* ret_type = llvm_type_to_string(curr->data.function_def.return_type);
            fprintf(g_ctx.output, "define %s @%s() {\n", ret_type, curr->data.function_def.name);
            gen_statement(curr->data.function_def.body);
            fprintf(g_ctx.output, "}\n\n");
        }
        curr = curr->next;
    }
}

void codegen_cleanup(void) {
}

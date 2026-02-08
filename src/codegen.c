#include "codegen.h"
#include "ast.h"
#include "symbols.h"
#include "error.h"
#include "memory.h"
#include <stdio.h>
#include <stdarg.h>

static CodeGenContext g_ctx;

void codegen_init(FILE* output) {
    g_ctx.output = output;
    g_ctx.next_reg_id = 0;
}

char* get_next_reg(void) {
    char* reg = (char*)arena_alloc(g_compiler_arena, 16);
    sprintf(reg, "r%d", g_ctx.next_reg_id++);
    return reg;
}

char* llvm_type_to_string(TypeInfo* type) {
    if (!type) return arena_strdup(g_compiler_arena, "i32");

    char* base_str = NULL;
    switch (type->base_type) {
        case TYPE_VOID:   base_str = "void"; break;
        case TYPE_CHAR:   base_str = "i8"; break;
        case TYPE_SHORT:  base_str = "i16"; break;
        case TYPE_INT:    base_str = "i32"; break;
        case TYPE_LONG:   base_str = "i64"; break;
        case TYPE_FLOAT:  base_str = "float"; break;
        case TYPE_DOUBLE: base_str = "double"; break;
        case TYPE_STRUCT:
        case TYPE_UNION: {
            char* buf = (char*)arena_alloc(g_compiler_arena, 128);
            sprintf(buf, "%%struct.%s", type->struct_name ? type->struct_name : "anon");
            base_str = buf;
            break;
        }
        default: base_str = "i32"; break;
    }

    if (type->pointer_level > 0) {
        char* ptr_str = (char*)arena_alloc(g_compiler_arena, strlen(base_str) + type->pointer_level + 1);
        strcpy(ptr_str, base_str);
        for (int i = 0; i < type->pointer_level; i++) strcat(ptr_str, "*");
        return ptr_str;
    }

    return arena_strdup(g_compiler_arena, base_str);
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

LLVMValue* gen_address(ASTNode* expr);

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
        case AST_MEMBER_ACCESS: {
            LLVMValue* addr = gen_address(expr);
            if (!addr) return NULL;
            
            TypeInfo* member_type = duplicate_type_info(addr->llvm_type);
            member_type->pointer_level--;
            
            char* reg = get_next_reg();
            char* type_str = llvm_type_to_string(member_type);
            emit_instruction("%%%s = load %s, %s* %%%s", reg, type_str, type_str, addr->name);
            return create_llvm_value(LLVM_VALUE_REGISTER, reg, member_type);
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
                    /* Handle assignment including member access */
                    LLVMValue* target_addr = gen_address(expr->data.binary_op.left);
                    if (!target_addr) {
                        error_report("Assignment to non-lvalue");
                        return NULL;
                    }
                    TypeInfo* target_type = duplicate_type_info(target_addr->llvm_type);
                    target_type->pointer_level--;
                    
                    char* type_str = llvm_type_to_string(target_type);
                    emit_instruction("store %s %s%s, %s* %%%s", 
                        type_str, 
                        right->type == LLVM_VALUE_REGISTER ? "%" : "", right->name,
                        type_str, target_addr->name);
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
                    
                    LLVMValue* target_addr = gen_address(expr->data.binary_op.left);
                    if (!target_addr) return NULL;
                    TypeInfo* target_type = duplicate_type_info(target_addr->llvm_type);
                    target_type->pointer_level--;

                    char* type_str = llvm_type_to_string(target_type);
                    char* res_reg = get_next_reg();
                    
                    /* Load, op, store */
                    emit_instruction("%%%s = %s %s %s%s, %s%s", 
                        res_reg, base_op, type_str,
                        left->type == LLVM_VALUE_REGISTER ? "%" : "", left->name,
                        right->type == LLVM_VALUE_REGISTER ? "%" : "", right->name);
                    
                    emit_instruction("store %s %%%s, %s* %%%s", 
                        type_str, res_reg, type_str, target_addr->name);
                    
                    return create_llvm_value(LLVM_VALUE_REGISTER, res_reg, target_type);
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
                emit_instruction("%%%s = xor %s %s%s, -1", reg, type_str,
                    operand->type == LLVM_VALUE_REGISTER ? "%" : "", operand->name);
                return create_llvm_value(LLVM_VALUE_REGISTER, reg, operand->llvm_type);
            }
            if (expr->data.unary_op.op == UOP_ADDR) {
                return gen_address(expr->data.unary_op.operand);
            }
            if (expr->data.unary_op.op == UOP_DEREF) {
                char* reg = get_next_reg();
                TypeInfo* res_type = duplicate_type_info(operand->llvm_type);
                res_type->pointer_level--;
                char* type_str = llvm_type_to_string(res_type);
                emit_instruction("%%%s = load %s, %s* %s%s", reg, type_str, type_str,
                    operand->type == LLVM_VALUE_REGISTER ? "%" : "", operand->name);
                return create_llvm_value(LLVM_VALUE_REGISTER, reg, res_type);
            }
            return operand;
        }
        default:
            return NULL;
    }
}

LLVMValue* gen_address(ASTNode* expr) {
    if (!expr) return NULL;
    switch (expr->type) {
        case AST_IDENTIFIER: {
            Symbol* sym = symbol_lookup(expr->data.identifier.name);
            if (!sym) return NULL;
            return create_llvm_value(LLVM_VALUE_REGISTER, sym->name, create_pointer_type(sym->type));
        }
        case AST_MEMBER_ACCESS: {
            ASTNode* object = expr->data.member_access.object;
            char* member_name = expr->data.member_access.member;
            int is_ptr = expr->data.member_access.is_pointer_access;
            
            LLVMValue* base_ptr = NULL;
            TypeInfo* struct_type = NULL;
            
            if (is_ptr) {
                base_ptr = gen_expression(object);
                struct_type = duplicate_type_info(base_ptr->llvm_type);
                struct_type->pointer_level--;
            } else {
                base_ptr = gen_address(object);
                struct_type = duplicate_type_info(base_ptr->llvm_type);
                struct_type->pointer_level--;
            }
            
            Symbol* member = struct_lookup_member(struct_type, member_name);
            if (!member) return NULL;
            
            char* res_reg = get_next_reg();
            char* struct_type_str = llvm_type_to_string(struct_type);
            emit_instruction("%%%s = getelementptr %s, %s* %s%s, i32 0, i32 %d",
                res_reg, struct_type_str, struct_type_str, 
                base_ptr->type == LLVM_VALUE_REGISTER ? "%" : "", base_ptr->name, 
                member->index);
            
            return create_llvm_value(LLVM_VALUE_REGISTER, res_reg, create_pointer_type(member->type));
        }
        case AST_UNARY_OP: {
            if (expr->data.unary_op.op == UOP_DEREF) {
                return gen_expression(expr->data.unary_op.operand);
            }
            break;
        }
        default: break;
    }
    return NULL;
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

    /* Define structs in LLVM IR */
    Symbol* curr_tag = g_tags;
    while (curr_tag) {
        if (curr_tag->type->base_type == TYPE_STRUCT || curr_tag->type->base_type == TYPE_UNION) {
            fprintf(g_ctx.output, "%%struct.%s = type { ", curr_tag->name);
            Symbol* m = curr_tag->type->struct_members;
            while (m) {
                fprintf(g_ctx.output, "%s%s", llvm_type_to_string(m->type), m->next ? ", " : "");
                m = m->next;
            }
            fprintf(g_ctx.output, " }\n");
        }
        curr_tag = curr_tag->next;
    }
    fprintf(g_ctx.output, "\n");

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
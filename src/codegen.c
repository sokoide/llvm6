#include "codegen.h"
#include "ast.h"
#include "symbols.h"
#include "error.h"
#include "memory.h"
#include <stdio.h>
#include <stdarg.h>

static CodeGenContext g_ctx;
static int g_next_label_id = 0;

typedef struct StringLiteral {
    char* label; char* value; int length; struct StringLiteral* next;
} StringLiteral;

static StringLiteral* g_string_literals = NULL;

void add_string_literal(char* label, char* value, int length) {
    StringLiteral* s = (StringLiteral*)arena_alloc(g_compiler_arena, sizeof(StringLiteral));
    s->label = label; s->value = arena_strdup(g_compiler_arena, value); s->length = length;
    s->next = g_string_literals; g_string_literals = s;
}

void codegen_init(FILE* output) {
    g_ctx.output = output; g_ctx.next_reg_id = 0; g_next_label_id = 0; g_string_literals = NULL;
}

char* get_next_label(const char* prefix) {
    char* label = (char*)arena_alloc(g_compiler_arena, 32);
    sprintf(label, "%s%d", prefix, g_next_label_id++); return label;
}

char* get_next_reg(void) {
    char* reg = (char*)arena_alloc(g_compiler_arena, 16);
    sprintf(reg, "r%d", g_ctx.next_reg_id++); return reg;
}

char* llvm_type_to_string(TypeInfo* type) {
    if (!type) return arena_strdup(g_compiler_arena, "i32");
    char* base_str = NULL;
    switch (type->base_type) {
        case TYPE_VOID:   base_str = (type->pointer_level > 0) ? "i8" : "void"; break;
        case TYPE_CHAR:   base_str = "i8"; break;
        case TYPE_SHORT:  base_str = "i16"; break;
        case TYPE_INT:    base_str = "i32"; break;
        case TYPE_LONG:   base_str = "i64"; break;
        case TYPE_FLOAT:  base_str = "float"; break;
        case TYPE_DOUBLE: base_str = "double"; break;
        case TYPE_UNSIGNED: base_str = "i32"; break;
        case TYPE_SIGNED:   base_str = "i32"; break;
        case TYPE_BOOL:     base_str = "i1"; break;
        case TYPE_STRUCT:
        case TYPE_UNION: {
            char* buf = (char*)arena_alloc(g_compiler_arena, 128);
            sprintf(buf, "%%struct.%s", type->struct_name ? type->struct_name : "anon");
            base_str = buf; break;
        }
        case TYPE_FUNCTION: {
            char* ret = llvm_type_to_string(type->return_type);
            char* buf = (char*)arena_alloc(g_compiler_arena, 512);
            sprintf(buf, "%s (", ret); ASTNode* p = type->parameters;
            while (p) {
                if (p->type == AST_VARIABLE_DECL) strcat(buf, llvm_type_to_string(p->data.variable_decl.type));
                else strcat(buf, "i32");
                if (p->next) strcat(buf, ", "); p = p->next;
            }
            strcat(buf, ")"); base_str = buf; break;
        }
        default: base_str = "i32"; break;
    }
    if (type->pointer_level > 0) {
        char* ptr_str = (char*)arena_alloc(g_compiler_arena, strlen(base_str) + type->pointer_level + 1);
        strcpy(ptr_str, base_str); for (int i = 0; i < type->pointer_level; i++) strcat(ptr_str, "*");
        return ptr_str;
    }
    return arena_strdup(g_compiler_arena, base_str);
}

void emit_instruction(const char* format, ...) {
    va_list args; va_start(args, format);
    fprintf(g_ctx.output, "  "); vfprintf(g_ctx.output, format, args); fprintf(g_ctx.output, "\n");
    va_end(args);
}

LLVMValue* create_llvm_value(LLVMValueType type, const char* name, TypeInfo* llvm_type) {
    LLVMValue* val = (LLVMValue*)arena_alloc(g_compiler_arena, sizeof(LLVMValue));
    val->type = type; val->name = arena_strdup(g_compiler_arena, name); val->llvm_type = llvm_type;
    return val;
}

static const char* val_prefix(LLVMValue* val) {
    if (!val) return "";
    switch (val->type) {
        case LLVM_VALUE_REGISTER: return "%";
        case LLVM_VALUE_GLOBAL:   return "@";
        case LLVM_VALUE_FUNCTION: return "@";
        default: return "";
    }
}

static char* format_operand(LLVMValue* val) {
    if (!val) return "";
    if (val->type == LLVM_VALUE_CONSTANT) {
        if (strcmp(val->name, "0") == 0 && val->llvm_type && val->llvm_type->pointer_level > 0) return "null";
        return val->name;
    }
    char* buf = (char*)arena_alloc(g_compiler_arena, strlen(val->name) + 2);
    sprintf(buf, "%s%s", val_prefix(val), val->name); return buf;
}

static LLVMValue* cast_to_type(LLVMValue* val, TypeInfo* target_type) {
    if (!val || !target_type) return val;
    char* src_type_str = llvm_type_to_string(val->llvm_type);
    char* dest_type_str = llvm_type_to_string(target_type);
    if (strcmp(src_type_str, dest_type_str) == 0) return val;

    /* Integer conversions */
    int src_size = get_type_size(val->llvm_type);
    int dest_size = get_type_size(target_type);
    
    if (src_size == 0 || dest_size == 0) return val;

    char* reg = get_next_reg();
    if (target_type->pointer_level > 0 && val->llvm_type->pointer_level == 0) {
        emit_instruction("%%%s = inttoptr %s %s to %s", reg, src_type_str, format_operand(val), dest_type_str);
    } else if (target_type->pointer_level == 0 && val->llvm_type->pointer_level > 0) {
        emit_instruction("%%%s = ptrtoint %s %s to %s", reg, src_type_str, format_operand(val), dest_type_str);
    } else if (src_size < dest_size) {
        emit_instruction("%%%s = sext %s %s to %s", reg, src_type_str, format_operand(val), dest_type_str);
    } else if (src_size > dest_size) {
        emit_instruction("%%%s = trunc %s %s to %s", reg, src_type_str, format_operand(val), dest_type_str);
    } else {
        emit_instruction("%%%s = bitcast %s %s to %s", reg, src_type_str, format_operand(val), dest_type_str);
    }
    return create_llvm_value(LLVM_VALUE_REGISTER, reg, target_type);
}

LLVMValue* gen_address(ASTNode* expr);

LLVMValue* gen_expression(ASTNode* expr) {
    if (!expr) return NULL;
    switch (expr->type) {
        case AST_IDENTIFIER: {
            Symbol* sym = symbol_lookup(expr->data.identifier.name);
            if (!sym) { error_report("Undefined identifier: %s", expr->data.identifier.name); return NULL; }
            if (sym->type->base_type == TYPE_FUNCTION && sym->type->pointer_level == 0) return create_llvm_value(LLVM_VALUE_FUNCTION, sym->name, create_pointer_type(sym->type));
            char* reg = get_next_reg(); char* type_str = llvm_type_to_string(sym->type);
            emit_instruction("%%%s = load %s, %s* %s%s", reg, type_str, type_str, sym->is_global ? "@" : "%", sym->name);
            return create_llvm_value(LLVM_VALUE_REGISTER, reg, sym->type);
        }
        case AST_CONSTANT: {
            char buf[32]; sprintf(buf, "%d", expr->data.constant.value.int_val);
            LLVMValue* val = create_llvm_value(LLVM_VALUE_CONSTANT, buf, create_type_info(expr->data.constant.const_type));
            val->data.constant_val = expr->data.constant.value.int_val; return val;
        }
        case AST_CAST: {
            LLVMValue* val = gen_expression(expr->data.cast_expr.operand); if (!val) return NULL;
            return create_llvm_value(val->type, val->name, expr->data.cast_expr.target_type);
        }
        case AST_STRING_LITERAL: {
            char* label = get_next_label(".str"); char* val = expr->data.string_literal.string; int len = expr->data.string_literal.length;
            if (val[0] == '\"') { val++; len -= 2; }
            int true_len = 0; for (int i = 0; i < len; i++) { if (val[i] == '\\') { if (i + 1 < len) { i++; true_len++; } } else true_len++; }
            add_string_literal(label, val, len); char* gep = (char*)arena_alloc(g_compiler_arena, 128);
            sprintf(gep, "getelementptr inbounds ([%d x i8], [%d x i8]* @%s, i32 0, i32 0)", true_len + 1, true_len + 1, label);
            return create_llvm_value(LLVM_VALUE_CONSTANT, gep, create_pointer_type(create_type_info(TYPE_CHAR)));
        }
        case AST_CONDITIONAL: {
            char* then_label = get_next_label("cond_then"); char* else_label = get_next_label("cond_else"); char* end_label = get_next_label("cond_end");
            LLVMValue* cond = gen_expression(expr->data.conditional_expr.condition); if (!cond) return NULL;
            char* cond_reg = get_next_reg(); emit_instruction("%%%s = icmp ne %s %s, %s", cond_reg, llvm_type_to_string(cond->llvm_type), format_operand(cond), (cond->llvm_type->pointer_level > 0) ? "null" : "0");
            emit_instruction("br i1 %%%s, label %%%s, label %%%s", cond_reg, then_label, else_label);
            fprintf(g_ctx.output, "%s:\n", then_label); LLVMValue* then_val = gen_expression(expr->data.conditional_expr.then_expr); emit_instruction("br label %%%s", end_label);
            fprintf(g_ctx.output, "%s:\n", else_label); LLVMValue* else_val = gen_expression(expr->data.conditional_expr.else_expr); emit_instruction("br label %%%s", end_label);
            fprintf(g_ctx.output, "%s:\n", end_label); if (!then_val || !else_val) return NULL;
            char* res_reg = get_next_reg(); char* res_type = llvm_type_to_string(then_val->llvm_type);
            emit_instruction("%%%s = phi %s [ %s, %%%s ], [ %s, %%%s ]", res_reg, res_type, format_operand(then_val), then_label, format_operand(else_val), else_label);
            return create_llvm_value(LLVM_VALUE_REGISTER, res_reg, then_val->llvm_type);
        }
        case AST_FUNCTION_CALL: {
            ASTNode* func_node = expr->data.function_call.function; LLVMValue* func_val = NULL; char* func_name = NULL; TypeInfo* ret_type = create_type_info(TYPE_INT);
            if (func_node->type == AST_IDENTIFIER) {
                func_name = func_node->data.identifier.name; Symbol* sym = symbol_lookup(func_name);
                if (sym && sym->type) {
                    if (sym->type->base_type == TYPE_FUNCTION && sym->type->pointer_level == 0) ret_type = sym->type->return_type;
                    else { func_val = gen_expression(func_node); if (func_val->llvm_type->base_type == TYPE_FUNCTION) ret_type = func_val->llvm_type->return_type; }
                }
            } else { func_val = gen_expression(func_node); if (func_val && func_val->llvm_type && func_val->llvm_type->base_type == TYPE_FUNCTION) ret_type = func_val->llvm_type->return_type; }
            int arg_count = 0; ASTNode* arg_curr = expr->data.function_call.arguments; while (arg_curr) { arg_count++; arg_curr = arg_curr->next; }
            LLVMValue** args = (LLVMValue**)malloc(sizeof(LLVMValue*) * arg_count); arg_curr = expr->data.function_call.arguments;
            for (int i = 0; i < arg_count; i++) { args[i] = gen_expression(arg_curr); arg_curr = arg_curr->next; }
            char* res_reg = (ret_type->base_type == TYPE_VOID && ret_type->pointer_level == 0) ? NULL : get_next_reg();
            char* ret_type_str = llvm_type_to_string(ret_type);
            fprintf(g_ctx.output, "  %s%s%s call %s ", res_reg ? "%" : "", res_reg ? res_reg : "", res_reg ? " =" : "", ret_type_str);
            if (!func_val && strcmp(func_name, "printf") == 0) fprintf(g_ctx.output, "(i8*, ...) ");
            if (func_val) fprintf(g_ctx.output, "%s", format_operand(func_val)); else fprintf(g_ctx.output, "@%s", func_name);
            fprintf(g_ctx.output, "(");
            for (int i = 0; i < arg_count; i++) { 
                if (!args[i]) { fprintf(g_ctx.output, "i32 0%s", (i < arg_count - 1) ? ", " : ""); continue; }
                fprintf(g_ctx.output, "%s %s%s", llvm_type_to_string(args[i]->llvm_type), format_operand(args[i]), (i < arg_count - 1) ? ", " : ""); 
            }
            fprintf(g_ctx.output, ")\n"); free(args); 
            if (res_reg) return create_llvm_value(LLVM_VALUE_REGISTER, res_reg, ret_type);
            else return NULL;
        }
                case AST_ARRAY_ACCESS: {
            LLVMValue* addr = gen_address(expr); if (!addr) return NULL;
            TypeInfo* elem_type = duplicate_type_info(addr->llvm_type); elem_type->pointer_level--;
            char* reg = get_next_reg(); emit_instruction("%%%s = load %s, %s* %s", reg, llvm_type_to_string(elem_type), llvm_type_to_string(elem_type), format_operand(addr));
            return create_llvm_value(LLVM_VALUE_REGISTER, reg, elem_type);
        }
        case AST_BINARY_OP: {
                    LLVMValue* left = NULL;
                    if (expr->data.binary_op.op != OP_ASSIGN && expr->data.binary_op.op < OP_ASSIGN) left = gen_expression(expr->data.binary_op.left);
                    LLVMValue* right = gen_expression(expr->data.binary_op.right);
                    if (expr->data.binary_op.op != OP_ASSIGN && !left) return NULL;
                    if (!right) return NULL;
        
                    /* Basic promotion to i64 */
                    if (left && left->llvm_type->base_type == TYPE_LONG && right->llvm_type->base_type != TYPE_LONG) {
                        char* reg = get_next_reg();
                        emit_instruction("%%%s = sext %s %s to i64", reg, llvm_type_to_string(right->llvm_type), format_operand(right));
                        right = create_llvm_value(LLVM_VALUE_REGISTER, reg, create_type_info(TYPE_LONG));
                    } else if (left && right->llvm_type->base_type == TYPE_LONG && left->llvm_type->base_type != TYPE_LONG) {
                        char* reg = get_next_reg();
                        emit_instruction("%%%s = sext %s %s to i64", reg, llvm_type_to_string(left->llvm_type), format_operand(left));
                        left = create_llvm_value(LLVM_VALUE_REGISTER, reg, create_type_info(TYPE_LONG));
                    }
        
                    const char* op_name = NULL; const char* icmp_cond = NULL;
        
            switch (expr->data.binary_op.op) {
                case OP_ADD: op_name = "add"; break; case OP_SUB: op_name = "sub"; break;
                case OP_MUL: op_name = "mul"; break; case OP_DIV: op_name = "sdiv"; break;
                case OP_MOD: op_name = "srem"; break; case OP_BITAND: op_name = "and"; break;
                case OP_BITOR:  op_name = "or";  break; case OP_XOR:    op_name = "xor"; break;
                case OP_LSHIFT: op_name = "shl"; break; case OP_RSHIFT: op_name = "ashr"; break;
                case OP_LT: icmp_cond = "slt"; break; case OP_GT: icmp_cond = "sgt"; break;
                case OP_LE: icmp_cond = "sle"; break; case OP_GE: icmp_cond = "sge"; break;
                case OP_EQ: icmp_cond = "eq"; break; case OP_NE: icmp_cond = "ne"; break;
                case OP_ASSIGN: {
                    LLVMValue* target_addr = gen_address(expr->data.binary_op.left); if (!target_addr) return NULL;
                    TypeInfo* target_type = duplicate_type_info(target_addr->llvm_type); target_type->pointer_level--;
                    LLVMValue* promoted_right = cast_to_type(right, target_type);
                    char* t_str = llvm_type_to_string(target_type);
                    emit_instruction("store %s %s, %s* %s", t_str, format_operand(promoted_right), t_str, format_operand(target_addr));
                    return promoted_right;
                }
                default: return NULL;
            }
            char* reg = get_next_reg(); char* type_str = llvm_type_to_string(left->llvm_type);
            if (icmp_cond) {
                emit_instruction("%%%s = icmp %s %s %s, %s", reg, icmp_cond, type_str, format_operand(left), format_operand(right));
                char* zext_reg = get_next_reg(); emit_instruction("%%%s = zext i1 %%%s to i32", zext_reg, reg);
                return create_llvm_value(LLVM_VALUE_REGISTER, zext_reg, create_type_info(TYPE_INT));
            } else {
                emit_instruction("%%%s = %s %s %s, %s", reg, op_name, type_str, format_operand(left), format_operand(right));
                return create_llvm_value(LLVM_VALUE_REGISTER, reg, left->llvm_type);
            }
        }
        case AST_UNARY_OP: {
            LLVMValue* operand = gen_expression(expr->data.unary_op.operand); if (!operand) return NULL;
            if (expr->data.unary_op.op == UOP_BITNOT) {
                char* reg = get_next_reg(); emit_instruction("%%%s = xor %s %s, -1", reg, llvm_type_to_string(operand->llvm_type), format_operand(operand));
                return create_llvm_value(LLVM_VALUE_REGISTER, reg, operand->llvm_type);
            }
            if (expr->data.unary_op.op == UOP_ADDR) return gen_address(expr->data.unary_op.operand);
            if (expr->data.unary_op.op == UOP_DEREF) {
                char* reg = get_next_reg(); TypeInfo* res_type = duplicate_type_info(operand->llvm_type); res_type->pointer_level--;
                emit_instruction("%%%s = load %s, %s* %s", reg, llvm_type_to_string(res_type), llvm_type_to_string(res_type), format_operand(operand));
                return create_llvm_value(LLVM_VALUE_REGISTER, reg, res_type);
            }
            if (expr->data.unary_op.op == UOP_NOT) {
                char* reg = get_next_reg();
                emit_instruction("%%%s = icmp eq %s %s, %s", reg, llvm_type_to_string(operand->llvm_type), format_operand(operand), (operand->llvm_type->pointer_level > 0) ? "null" : "0");
                /* zext to i32 for C boolean */
                char* zext_reg = get_next_reg();
                emit_instruction("%%%s = zext i1 %%%s to i32", zext_reg, reg);
                return create_llvm_value(LLVM_VALUE_REGISTER, zext_reg, create_type_info(TYPE_INT));
            }
            return operand;
        }
        case AST_MEMBER_ACCESS: {
            LLVMValue* addr = gen_address(expr); if (!addr) return NULL;
            TypeInfo* member_type = duplicate_type_info(addr->llvm_type); member_type->pointer_level--;
            char* reg = get_next_reg(); emit_instruction("%%%s = load %s, %s* %s", reg, llvm_type_to_string(member_type), llvm_type_to_string(member_type), format_operand(addr));
            return create_llvm_value(LLVM_VALUE_REGISTER, reg, member_type);
        }
        default: return NULL;
    }
}

LLVMValue* gen_address(ASTNode* expr) {
    if (!expr) return NULL;
    switch (expr->type) {
        case AST_IDENTIFIER: {
            Symbol* sym = symbol_lookup(expr->data.identifier.name); if (!sym) return NULL;
            return create_llvm_value(sym->is_global ? LLVM_VALUE_GLOBAL : LLVM_VALUE_REGISTER, sym->name, create_pointer_type(sym->type));
        }
        case AST_ARRAY_ACCESS: {
            LLVMValue* base = gen_expression(expr->data.array_access.array);
            LLVMValue* index = gen_expression(expr->data.array_access.index);
            if (!base || !index || !base->llvm_type) return NULL;
            char* res_reg = get_next_reg();
            TypeInfo* pointed_to = duplicate_type_info(base->llvm_type); pointed_to->pointer_level--;
            char* p_str = llvm_type_to_string(pointed_to);
            emit_instruction("%%%s = getelementptr %s, %s %s, %s %s",
                res_reg, p_str, llvm_type_to_string(base->llvm_type), format_operand(base),
                llvm_type_to_string(index->llvm_type), format_operand(index));
            return create_llvm_value(LLVM_VALUE_REGISTER, res_reg, base->llvm_type);
        }
        case AST_MEMBER_ACCESS: {
            ASTNode* object = expr->data.member_access.object; char* member_name = expr->data.member_access.member;
            int is_ptr = expr->data.member_access.is_pointer_access; LLVMValue* base_ptr = NULL; TypeInfo* struct_type = NULL;
            if (is_ptr) { 
                base_ptr = gen_expression(object); 
                if (!base_ptr || !base_ptr->llvm_type) return NULL;
                struct_type = duplicate_type_info(base_ptr->llvm_type); struct_type->pointer_level--; 
            }
            else { 
                base_ptr = gen_address(object); 
                if (!base_ptr || !base_ptr->llvm_type) return NULL;
                struct_type = duplicate_type_info(base_ptr->llvm_type); struct_type->pointer_level--; 
            }
            Symbol* member = struct_lookup_member(struct_type, member_name); if (!member) return NULL;
            char* res_reg = get_next_reg(); char* struct_type_str = llvm_type_to_string(struct_type);
            emit_instruction("%%%s = getelementptr %s, %s* %s, i32 0, i32 %d", res_reg, struct_type_str, struct_type_str, format_operand(base_ptr), member->index);
            return create_llvm_value(LLVM_VALUE_REGISTER, res_reg, create_pointer_type(member->type));
        }
        case AST_UNARY_OP: { if (expr->data.unary_op.op == UOP_DEREF) return gen_expression(expr->data.unary_op.operand); break; }
        default: break;
    }
    return NULL;
}

void gen_statement(ASTNode* stmt) {
    if (!stmt) return;
    switch (stmt->type) {
        case AST_COMPOUND_STMT: { ASTNode* curr = stmt->data.compound_stmt.statements; while (curr) { gen_statement(curr); curr = curr->next; } break; }
        case AST_VARIABLE_DECL: {
            Symbol* sym = create_symbol(stmt->data.variable_decl.name, stmt->data.variable_decl.type); symbol_add_local(sym);
            char* type_str = llvm_type_to_string(sym->type); 
            
            char* unique_name = (char*)arena_alloc(g_compiler_arena, strlen(sym->name) + 16);
            sprintf(unique_name, "%s.%d", sym->name, g_ctx.next_reg_id++);
            sym->name = unique_name;

            emit_instruction("%%%s = alloca %s", sym->name, type_str);
            if (stmt->data.variable_decl.initializer) {
                LLVMValue* init = gen_expression(stmt->data.variable_decl.initializer);
                if (init) {
                    LLVMValue* casted = cast_to_type(init, sym->type);
                    emit_instruction("store %s %s, %s* %%%s", type_str, format_operand(casted), type_str, sym->name);
                }
            }
            break;
        }
        case AST_IF_STMT: {
            char* then_label = get_next_label("if_then"); char* else_label = stmt->data.if_stmt.else_stmt ? get_next_label("if_else") : NULL; char* end_label = get_next_label("if_end");
            LLVMValue* cond = gen_expression(stmt->data.if_stmt.condition); if (!cond) break;
            char* cond_reg = get_next_reg(); emit_instruction("%%%s = icmp ne %s %s, %s", cond_reg, llvm_type_to_string(cond->llvm_type), format_operand(cond), (cond->llvm_type->pointer_level > 0) ? "null" : "0");
            emit_instruction("br i1 %%%s, label %%%s, label %%%s", cond_reg, then_label, else_label ? else_label : end_label);
            fprintf(g_ctx.output, "%s:\n", then_label); gen_statement(stmt->data.if_stmt.then_stmt); emit_instruction("br label %%%s", end_label);
            if (else_label) { fprintf(g_ctx.output, "%s:\n", else_label); gen_statement(stmt->data.if_stmt.else_stmt); emit_instruction("br label %%%s", end_label); }
            fprintf(g_ctx.output, "%s:\n", end_label); break;
        }
        case AST_WHILE_STMT: {
            char* cond_label = get_next_label("while_cond"); char* body_label = get_next_label("while_body"); char* end_label = get_next_label("while_end");
            emit_instruction("br label %%%s", cond_label); fprintf(g_ctx.output, "%s:\n", cond_label);
            LLVMValue* cond = gen_expression(stmt->data.while_stmt.condition); if (!cond) break;
            char* cond_reg = get_next_reg(); emit_instruction("%%%s = icmp ne %s %s, %s", cond_reg, llvm_type_to_string(cond->llvm_type), format_operand(cond), (cond->llvm_type->pointer_level > 0) ? "null" : "0");
            emit_instruction("br i1 %%%s, label %%%s, label %%%s", cond_reg, body_label, end_label);
            fprintf(g_ctx.output, "%s:\n", body_label); gen_statement(stmt->data.while_stmt.body); emit_instruction("br label %%%s", cond_label);
            fprintf(g_ctx.output, "%s:\n", end_label); break;
        }
        case AST_FOR_STMT: {
            char* cond_label = get_next_label("for_cond"); char* body_label = get_next_label("for_body"); char* incr_label = get_next_label("for_incr"); char* end_label = get_next_label("for_end");
            gen_statement(stmt->data.for_stmt.init); emit_instruction("br label %%%s", cond_label); fprintf(g_ctx.output, "%s:\n", cond_label);
            ASTNode* cond_node = stmt->data.for_stmt.condition; if (cond_node && cond_node->type == AST_EXPRESSION_STMT) cond_node = cond_node->data.return_stmt.expression;
            if (cond_node) {
                LLVMValue* cond = gen_expression(cond_node); if (cond) { char* cond_reg = get_next_reg(); emit_instruction("%%%s = icmp ne %s %s, %s", cond_reg, llvm_type_to_string(cond->llvm_type), format_operand(cond), (cond->llvm_type->pointer_level > 0) ? "null" : "0"); emit_instruction("br i1 %%%s, label %%%s, label %%%s", cond_reg, body_label, end_label); } else emit_instruction("br label %%%s", body_label);
            } else emit_instruction("br label %%%s", body_label);
            fprintf(g_ctx.output, "%s:\n", body_label); gen_statement(stmt->data.for_stmt.body); emit_instruction("br label %%%s", incr_label);
            fprintf(g_ctx.output, "%s:\n", incr_label); if (stmt->data.for_stmt.update) { ASTNode* u = stmt->data.for_stmt.update; if (u->type == AST_EXPRESSION_STMT) u = u->data.return_stmt.expression; gen_expression(u); }
            emit_instruction("br label %%%s", cond_label); fprintf(g_ctx.output, "%s:\n", end_label); break;
        }
        case AST_RETURN_STMT: {
            LLVMValue* val = gen_expression(stmt->data.return_stmt.expression);
            if (val) {
                LLVMValue* casted = cast_to_type(val, g_ctx.current_function_return_type);
                emit_instruction("ret %s %s", llvm_type_to_string(g_ctx.current_function_return_type), format_operand(casted));
            } else {
                emit_instruction("ret void");
            }
            break;
        }
        case AST_EXPRESSION_STMT: { gen_expression(stmt->data.return_stmt.expression); break; }
        default: break;
    }
}

void codegen_run(ASTNode* ast) {
    if (!ast) return;
    fprintf(g_ctx.output, "; Generated LLVM IR\ntarget triple = \"arm64-apple-darwin\"\n\n");
    
    /* Define structs in LLVM IR */
    TypeInfo* curr_type = g_all_structs;
    while (curr_type) {
        if (curr_type->base_type == TYPE_STRUCT || curr_type->base_type == TYPE_UNION) {
            fprintf(g_ctx.output, "%%struct.%s = type { ", curr_type->struct_name);
            Symbol* m = curr_type->struct_members;
            while (m) { fprintf(g_ctx.output, "%s%s", llvm_type_to_string(m->type), m->next ? ", " : ""); m = m->next; }
            fprintf(g_ctx.output, " }\n");
        }
        curr_type = curr_type->next;
    }
    fprintf(g_ctx.output, "\n");
    /* Emit global variable declarations */
    Symbol* g_sym = g_global_symbols;
    while (g_sym) {
        if (g_sym->type->base_type == TYPE_FUNCTION && g_sym->type->storage_class != STORAGE_TYPEDEF) {
            /* Check if this function is defined in our AST */
            int defined = 0;
            ASTNode* check = ast;
            while (check) {
                if (check->type == AST_FUNCTION_DEF && strcmp(check->data.function_def.name, g_sym->name) == 0) {
                    defined = 1; break;
                }
                check = check->next;
            }
            if (!defined) {
                /* Deduplicate */
                int found = 0; Symbol* prev = g_global_symbols;
                while (prev != g_sym) { if (strcmp(prev->name, g_sym->name) == 0) { found = 1; break; } prev = prev->next; }
                if (!found) {
                    char* ret_type = llvm_type_to_string(g_sym->type->return_type);
                    fprintf(g_ctx.output, "declare %s @%s(", ret_type, g_sym->name);
                    /* For declare, we can just use ... or full types. Let's use ... for simplicity if it's external */
                    fprintf(g_ctx.output, "...)\n");
                }
            }
        } else if (g_sym->type->base_type != TYPE_FUNCTION && g_sym->type->storage_class != STORAGE_TYPEDEF) {
            int found = 0; Symbol* prev = g_global_symbols;
            while (prev != g_sym) { if (strcmp(prev->name, g_sym->name) == 0) { found = 1; break; } prev = prev->next; }
            if (!found) {
                char* t_str = llvm_type_to_string(g_sym->type);
                if (g_sym->type->storage_class == STORAGE_EXTERN) fprintf(g_ctx.output, "@%s = external global %s\n", g_sym->name, t_str);
                else fprintf(g_ctx.output, "@%s = global %s %s\n", g_sym->name, t_str, (g_sym->type->pointer_level > 0) ? "null" : "zeroinitializer");
            }
        }
        g_sym = g_sym->next;
    }
    fprintf(g_ctx.output, "\n");
    ASTNode* curr = ast;
    while (curr) {
        if (curr->type == AST_FUNCTION_DEF) {
            TypeInfo* func_type = create_function_type(curr->data.function_def.return_type, curr->data.function_def.parameters);
            Symbol* sym = create_symbol(curr->data.function_def.name, func_type); sym->is_global = 1; symbol_add_global(sym);
        }
        curr = curr->next;
    }
    curr = ast;
    while (curr) {
        if (curr->type == AST_FUNCTION_DEF) {
            symbol_clear_locals();
            g_ctx.current_function_return_type = curr->data.function_def.return_type;
            fprintf(g_ctx.output, "define %s @%s(", llvm_type_to_string(curr->data.function_def.return_type), curr->data.function_def.name);
            ASTNode* param = curr->data.function_def.parameters; int p_idx = 0;
            while (param) { fprintf(g_ctx.output, "%s %%p%d%s", llvm_type_to_string(param->data.variable_decl.type), p_idx++, param->next ? ", " : ""); param = param->next; }
            fprintf(g_ctx.output, ") {\n");
            param = curr->data.function_def.parameters; p_idx = 0;
            while (param) {
                Symbol* sym = create_symbol(param->data.variable_decl.name, param->data.variable_decl.type); symbol_add_local(sym);
                char* p_type = llvm_type_to_string(sym->type);
                emit_instruction("%%%s = alloca %s", sym->name, p_type);
                emit_instruction("store %s %%p%d, %s* %%%s", p_type, p_idx++, p_type, sym->name);
                param = param->next;
            }
            gen_statement(curr->data.function_def.body);
            if (curr->data.function_def.return_type->base_type == TYPE_VOID && curr->data.function_def.return_type->pointer_level == 0) {
                emit_instruction("ret void");
            } else {
                emit_instruction("ret %s %s", llvm_type_to_string(curr->data.function_def.return_type),
                    (curr->data.function_def.return_type->pointer_level > 0) ? "null" : "0");
            }
            fprintf(g_ctx.output, "}\n\n");
        }
        curr = curr->next;
    }
    StringLiteral* sl = g_string_literals;
    while (sl) {
        int true_len = 0; for (int i = 0; i < sl->length; i++) { if (sl->value[i] == '\\') { if (i + 1 < sl->length) { i++; true_len++; } } else true_len++; }
        fprintf(g_ctx.output, "@%s = private unnamed_addr constant [%d x i8] c\"", sl->label, true_len + 1);
        for (int i = 0; i < sl->length; i++) {
            if (sl->value[i] == '\\') {
                if (i + 1 < sl->length) {
                    if (sl->value[i+1] == 'n') fprintf(g_ctx.output, "\\0A");
                    else if (sl->value[i+1] == 't') fprintf(g_ctx.output, "\\09");
                    else if (sl->value[i+1] == 'r') fprintf(g_ctx.output, "\\0D");
                    else if (sl->value[i+1] == '0') fprintf(g_ctx.output, "\\00");
                    else fprintf(g_ctx.output, "%c", sl->value[i+1]);
                    i++;
                }
            } else if (sl->value[i] == '\"') fprintf(g_ctx.output, "\\22");
            else fprintf(g_ctx.output, "%c", sl->value[i]);
        }
        fprintf(g_ctx.output, "\\00\", align 1\n"); sl = sl->next;
    }
}

void codegen_cleanup(void) {}
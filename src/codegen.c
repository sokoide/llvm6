#include "codegen.h"
#include "ast.h"
#include "symbols.h"
#include "error.h"
#include "memory.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

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
    g_ctx.output = output; g_ctx.alloca_file = NULL; g_ctx.next_reg_id = 0; g_next_label_id = 0; g_string_literals = NULL;
    g_ctx.labels = NULL; g_ctx.current_break_label = NULL; g_ctx.current_continue_label = NULL;
}

char* get_next_label(const char* prefix) {
    char* label = (char*)arena_alloc(g_compiler_arena, 32);
    sprintf(label, "%s%d", prefix, g_next_label_id++); return label;
}

/* Helper for collecting switch cases */
typedef struct CaseNode {
    int value;
    char* label;
    int is_default;
    struct CaseNode* next;
} CaseNode;

static CaseNode* collect_cases(ASTNode* node, CaseNode* head) {
    if (!node) return head;
    if (node->type == AST_CASE_STMT || node->type == AST_DEFAULT_STMT) {
        CaseNode* n = (CaseNode*)arena_alloc(g_compiler_arena, sizeof(CaseNode));
        if (node->type == AST_CASE_STMT) {
            n->value = evaluate_constant_node(node->data.case_stmt.value);
            n->is_default = 0;
            n->label = get_next_label("case");
        } else {
            n->is_default = 1;
            n->label = get_next_label("default");
        }
        node->data.case_stmt.label = n->label;
        n->next = head;
        head = n;
        return collect_cases(node->data.case_stmt.statement, head);
    }
    if (node->type == AST_SWITCH_STMT) return head;
    if (node->type == AST_COMPOUND_STMT) {
        ASTNode* curr = node->data.compound_stmt.statements;
        while (curr) { head = collect_cases(curr, head); curr = curr->next; }
    } else if (node->type == AST_IF_STMT) {
        head = collect_cases(node->data.if_stmt.then_stmt, head);
        head = collect_cases(node->data.if_stmt.else_stmt, head);
    } else if (node->type == AST_WHILE_STMT || node->type == AST_DO_WHILE_STMT) {
        head = collect_cases(node->data.while_stmt.body, head);
    } else if (node->type == AST_FOR_STMT) {
        head = collect_cases(node->data.for_stmt.body, head);
    }
    return head;
}

static char* get_user_label(const char* name) {
    if (!name) {
        /* Generate a unique label for NULL names to avoid crash */
        char* fallback = get_next_label("null_label");
        return fallback;
    }
    LabelEntry* curr = g_ctx.labels;
    while (curr) { if (strcmp(curr->name, name) == 0) return curr->llvm_label; curr = curr->next; }
    LabelEntry* entry = (LabelEntry*)arena_alloc(g_compiler_arena, sizeof(LabelEntry));
    entry->name = arena_strdup(g_compiler_arena, name);
    entry->llvm_label = (char*)arena_alloc(g_compiler_arena, strlen(name) + 16);
    sprintf(entry->llvm_label, "user_label_%s", name);
    entry->next = g_ctx.labels; g_ctx.labels = entry;
    return entry->llvm_label;
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
        case TYPE_UNSIGNED: base_str = "i32"; break;  /* unsigned int */
        case TYPE_SIGNED:   base_str = "i32"; break;  /* signed int */
        case TYPE_FLOAT:  base_str = "float"; break;
        case TYPE_DOUBLE: base_str = "double"; break;
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
                if (p->next || type->is_variadic) strcat(buf, ", "); p = p->next;
            }
            if (type->is_variadic) strcat(buf, "...");
            strcat(buf, ")"); base_str = buf; break;
        }
        case TYPE_ARRAY:
            /* Array base type is in return_type */
            base_str = llvm_type_to_string(type->return_type);
            break;
        default: base_str = "i32"; break;
    }
    if (type->array_size > 0) {
        char* arr_str = (char*)arena_alloc(g_compiler_arena, strlen(base_str) + 32);
        sprintf(arr_str, "[%d x %s]", type->array_size, base_str);
        return arr_str;
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
    FILE* target = g_ctx.output;
    if (g_ctx.alloca_file && strstr(format, "= alloca")) {
        target = g_ctx.alloca_file;
    }
    fprintf(target, "  "); vfprintf(target, format, args); fprintf(target, "\n");
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
    if (!val || !target_type) {
        fprintf(stderr, "cast_to_type: NULL val or target_type\n");
        return val;
    }
    char* src_type_str = llvm_type_to_string(val->llvm_type);
    char* dest_type_str = llvm_type_to_string(target_type);

    int src_size = get_type_size(val->llvm_type); int dest_size = get_type_size(target_type);

    fprintf(stderr, "DEBUG: cast_to_type %s(ptr=%d, sz=%d) -> %s(ptr=%d, sz=%d)\n",
            src_type_str, val->llvm_type->pointer_level, src_size,
            dest_type_str, target_type->pointer_level, dest_size);

    if (strcmp(src_type_str, dest_type_str) == 0) return val;

    /* Handle void type */
    if (target_type->base_type == TYPE_VOID && target_type->pointer_level == 0) {
        return val;
    }

    /* For pointer to pointer casts, use bitcast - PRIORITIZE THIS */
    if (val->llvm_type->pointer_level > 0 && target_type->pointer_level > 0) {
        char* reg = get_next_reg();
        emit_instruction("%%%s = bitcast %s %s to %s", reg, src_type_str, format_operand(val), dest_type_str);
        return create_llvm_value(LLVM_VALUE_REGISTER, reg, target_type);
    }

    /* For integer to pointer or pointer to integer casts */
    if (target_type->pointer_level > 0 && val->llvm_type->pointer_level == 0) {
        char* reg = get_next_reg();
        emit_instruction("%%%s = inttoptr %s %s to %s", reg, src_type_str, format_operand(val), dest_type_str);
        return create_llvm_value(LLVM_VALUE_REGISTER, reg, target_type);
    } else if (target_type->pointer_level == 0 && val->llvm_type->pointer_level > 0) {
        char* reg = get_next_reg();
        emit_instruction("%%%s = ptrtoint %s %s to %s", reg, src_type_str, format_operand(val), dest_type_str);
        return create_llvm_value(LLVM_VALUE_REGISTER, reg, target_type);
    }

    /* For integer to integer casts, use sext/trunc based on size */
    if (src_size == 0 || dest_size == 0) {
        return val;
    }

    char* reg = get_next_reg();
    if (src_size < dest_size) {
        emit_instruction("%%%s = sext %s %s to %s", reg, src_type_str, format_operand(val), dest_type_str);
    } else if (src_size > dest_size) {
        emit_instruction("%%%s = trunc %s %s to %s", reg, src_type_str, format_operand(val), dest_type_str);
    } else {
        emit_instruction("%%%s = bitcast %s %s to %s", reg, src_type_str, format_operand(val), dest_type_str);
    }
    return create_llvm_value(LLVM_VALUE_REGISTER, reg, target_type);
}

LLVMValue* gen_address(ASTNode* expr);

/* Get type of expression without generating code */
static TypeInfo* get_expression_type(ASTNode* expr) {
    if (!expr) return create_type_info(TYPE_INT);
    switch (expr->type) {
        case AST_IDENTIFIER: {
            Symbol* sym = symbol_lookup(expr->data.identifier.name);
            if (sym && sym->type) return sym->type;
            return create_type_info(TYPE_INT);
        }
        case AST_CONSTANT:
            return expr->data_type ? expr->data_type : create_type_info(TYPE_INT);
        case AST_STRING_LITERAL:
            return create_pointer_type(create_type_info(TYPE_CHAR));
        case AST_UNARY_OP: {
            TypeInfo* operand_type = get_expression_type(expr->data.unary_op.operand);
            if (expr->data.unary_op.op == UOP_ADDR) {
                return create_pointer_type(operand_type);
            } else if (expr->data.unary_op.op == UOP_DEREF) {
                if (operand_type->pointer_level > 0) {
                    TypeInfo* base = duplicate_type_info(operand_type);
                    base->pointer_level--;
                    return base;
                }
                return create_type_info(TYPE_INT);
            } else if (expr->data.unary_op.op == UOP_SIZEOF) {
                return create_type_info(TYPE_INT);
            }
            return operand_type;
        }
        case AST_BINARY_OP:
            return get_expression_type(expr->data.binary_op.left);
        case AST_CONDITIONAL:
            return get_expression_type(expr->data.conditional_expr.then_expr);
        case AST_FUNCTION_CALL: {
            if (expr->data.function_call.function->type == AST_IDENTIFIER) {
                Symbol* sym = symbol_lookup(expr->data.function_call.function->data.identifier.name);
                if (sym && sym->type && sym->type->return_type) return sym->type->return_type;
            }
            return create_type_info(TYPE_INT);
        }
        case AST_ARRAY_ACCESS: {
            TypeInfo* base_type = get_expression_type(expr->data.array_access.array);
            if (base_type->pointer_level > 0) {
                TypeInfo* elem_type = duplicate_type_info(base_type);
                elem_type->pointer_level--;
                return elem_type;
            }
            return create_type_info(TYPE_INT);
        }
        case AST_MEMBER_ACCESS:
        case AST_CAST:
            return expr->data_type ? expr->data_type : create_type_info(TYPE_INT);
        default:
            return create_type_info(TYPE_INT);
    }
}

LLVMValue* gen_expression(ASTNode* expr) {
    if (!expr) return NULL;
    switch (expr->type) {
        case AST_IDENTIFIER: {
            Symbol* sym = symbol_lookup(expr->data.identifier.name);
            if (!sym) { error_report("Undefined identifier: %s", expr->data.identifier.name); return NULL; }
            if (sym->is_enum_constant) {
                char* val_str = (char*)arena_alloc(g_compiler_arena, 16);
                sprintf(val_str, "%d", sym->enum_value);
                return create_llvm_value(LLVM_VALUE_CONSTANT, val_str, sym->type);
            }
            if (sym->type->base_type == TYPE_FUNCTION && sym->type->pointer_level == 0) return create_llvm_value(LLVM_VALUE_FUNCTION, sym->name, create_pointer_type(sym->type));
            if (sym->type->array_size > 0) {
                char* reg = get_next_reg(); char* arr_type = llvm_type_to_string(sym->type);
                /* Create element type - use return_type which is the element type */
                TypeInfo* elem_type = duplicate_type_info(sym->type->return_type);
                emit_instruction("%%%s = getelementptr %s, %s* %s%s, i32 0, i32 0", reg, arr_type, arr_type, sym->is_global ? "@" : "%", sym->name);
                return create_llvm_value(LLVM_VALUE_REGISTER, reg, create_pointer_type(elem_type));
            }
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
            return cast_to_type(val, expr->data.cast_expr.target_type);
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
            char* then_label = get_next_label("cond_then");
            char* else_label = get_next_label("cond_else");
            char* end_label = get_next_label("cond_end");

            LLVMValue* cond = gen_expression(expr->data.conditional_expr.condition);
            if (!cond) return NULL;

            /* Determine common type using type inference (no code generation) */
            TypeInfo* then_type = get_expression_type(expr->data.conditional_expr.then_expr);
            TypeInfo* else_type = get_expression_type(expr->data.conditional_expr.else_expr);
            TypeInfo* common_type = then_type;
            int then_size = get_type_size(then_type);
            int else_size = get_type_size(else_type);
            if (else_size > then_size) {
                common_type = else_type;
            }

            char* cond_reg = get_next_reg();
            emit_instruction("%%%s = icmp ne %s %s, %s", cond_reg, llvm_type_to_string(cond->llvm_type), format_operand(cond), (cond->llvm_type->pointer_level > 0) ? "null" : "0");
            emit_instruction("br i1 %%%s, label %%%s, label %%%s", cond_reg, then_label, else_label);

            /* Then branch */
            fprintf(g_ctx.output, "%s:\n", then_label);
            LLVMValue* then_val = gen_expression(expr->data.conditional_expr.then_expr);
            char* then_phi_label = then_label;
            int then_needs_cast = (strcmp(llvm_type_to_string(then_val->llvm_type), llvm_type_to_string(common_type)) != 0);
            if (then_needs_cast) {
                then_phi_label = get_next_label("cond_then_cast");
                emit_instruction("br label %%%s", then_phi_label);
                fprintf(g_ctx.output, "%s:\n", then_phi_label);
                then_val = cast_to_type(then_val, common_type);
            }
            emit_instruction("br label %%%s", end_label);

            /* Else branch */
            fprintf(g_ctx.output, "%s:\n", else_label);
            LLVMValue* else_val = gen_expression(expr->data.conditional_expr.else_expr);
            char* else_phi_label = else_label;
            int else_needs_cast = (strcmp(llvm_type_to_string(else_val->llvm_type), llvm_type_to_string(common_type)) != 0);
            if (else_needs_cast) {
                else_phi_label = get_next_label("cond_else_cast");
                emit_instruction("br label %%%s", else_phi_label);
                fprintf(g_ctx.output, "%s:\n", else_phi_label);
                else_val = cast_to_type(else_val, common_type);
            }
            emit_instruction("br label %%%s", end_label);

            /* End block with phi */
            fprintf(g_ctx.output, "%s:\n", end_label);
            if (!then_val || !else_val) return NULL;
            char* res_reg = get_next_reg();
            char* res_type = llvm_type_to_string(common_type);
            emit_instruction("%%%s = phi %s [ %s, %%%s ], [ %s, %%%s ]", res_reg, res_type, format_operand(then_val), then_phi_label, format_operand(else_val), else_phi_label);
            return create_llvm_value(LLVM_VALUE_REGISTER, res_reg, common_type);
        }
        case AST_FUNCTION_CALL: {
            ASTNode* func_node = expr->data.function_call.function; LLVMValue* func_val = NULL; char* func_name = NULL; TypeInfo* ret_type = create_type_info(TYPE_INT);
            TypeInfo* func_type = NULL;
            if (func_node->type == AST_IDENTIFIER) {
                func_name = func_node->data.identifier.name; Symbol* sym = symbol_lookup(func_name);
                if (sym && sym->type) {
                    if (sym->type->base_type == TYPE_FUNCTION && sym->type->pointer_level == 0) {
                        func_type = sym->type; ret_type = sym->type->return_type;
                    } else {
                        func_val = gen_expression(func_node);
                        if (func_val && func_val->llvm_type && func_val->llvm_type->base_type == TYPE_FUNCTION) {
                            func_type = func_val->llvm_type; ret_type = func_type->return_type;
                        }
                    }
                } else ret_type = create_type_info(TYPE_INT);
            } else {
                func_val = gen_expression(func_node);
                if (func_val && func_val->llvm_type && func_val->llvm_type->base_type == TYPE_FUNCTION) {
                    func_type = func_val->llvm_type; ret_type = func_type->return_type;
                }
            }
            int arg_count = 0; ASTNode* arg_curr = expr->data.function_call.arguments; while (arg_curr) { arg_count++; arg_curr = arg_curr->next; }
            LLVMValue** args = (LLVMValue**)malloc(sizeof(LLVMValue*) * arg_count); arg_curr = expr->data.function_call.arguments;
            for (int i = 0; i < arg_count; i++) { args[i] = gen_expression(arg_curr); arg_curr = arg_curr->next; }
            /* Handle builtin functions before emitting call instruction */
            if (!func_val && (strcmp(func_name, "__builtin_va_start") == 0 || strcmp(func_name, "__builtin_va_end") == 0)) {
                LLVMValue* ap_addr = gen_address(expr->data.function_call.arguments);
                char* ap_addr_str = format_operand(ap_addr);
                char* cast_reg = get_next_reg();
                emit_instruction("%%%s = bitcast %s %s to i8*", cast_reg, llvm_type_to_string(ap_addr->llvm_type), ap_addr_str);
                emit_instruction("call void @llvm.%s(i8* %%%s)", strcmp(func_name, "__builtin_va_start") == 0 ? "va_start" : "va_end", cast_reg);
                free(args); return NULL;
            }

            if (!func_val && strcmp(func_name, "__builtin_memcpy") == 0) {
                LLVMValue* dst = gen_expression(expr->data.function_call.arguments);
                LLVMValue* src = gen_expression(expr->data.function_call.arguments->next);
                LLVMValue* n = gen_expression(expr->data.function_call.arguments->next->next);
                char* dst_cast = get_next_reg();
                char* src_cast = get_next_reg();
                TypeInfo* i64_type = create_type_info(TYPE_LONG);
                LLVMValue* n_i64 = cast_to_type(n, i64_type);
                emit_instruction("%%%s = bitcast %s %s to i8*", dst_cast, llvm_type_to_string(dst->llvm_type), format_operand(dst));
                emit_instruction("%%%s = bitcast %s %s to i8*", src_cast, llvm_type_to_string(src->llvm_type), format_operand(src));
                emit_instruction("call void @llvm.memcpy.p0i8.p0i8.i64(i8* %%%s, i8* %%%s, i64 %s, i1 false)", dst_cast, src_cast, format_operand(n_i64));
                free(args);
                return create_llvm_value(LLVM_VALUE_REGISTER, dst_cast, create_type_info(TYPE_INT));
            }

            if (!func_val && strcmp(func_name, "__builtin_memset") == 0) {
                LLVMValue* dst = gen_expression(expr->data.function_call.arguments);
                LLVMValue* c = gen_expression(expr->data.function_call.arguments->next);
                LLVMValue* n = gen_expression(expr->data.function_call.arguments->next->next);
                char* dst_cast = get_next_reg();
                TypeInfo* i64_type = create_type_info(TYPE_LONG);
                LLVMValue* n_i64 = cast_to_type(n, i64_type);
                emit_instruction("%%%s = bitcast %s %s to i8*", dst_cast, llvm_type_to_string(dst->llvm_type), format_operand(dst));
                emit_instruction("call void @llvm.memset.p0i8.i64(i8* %%%s, i8 %s, i64 %s, i1 false)", dst_cast, format_operand(c), format_operand(n_i64));
                free(args);
                return create_llvm_value(LLVM_VALUE_REGISTER, dst_cast, create_type_info(TYPE_INT));
            }

            if (!func_val && strcmp(func_name, "__builtin_memcmp") == 0) {
                LLVMValue* s1 = gen_expression(expr->data.function_call.arguments);
                LLVMValue* s2 = gen_expression(expr->data.function_call.arguments->next);
                LLVMValue* n = gen_expression(expr->data.function_call.arguments->next->next);
                char* s1_cast = get_next_reg();
                char* s2_cast = get_next_reg();
                char* result = get_next_reg();
                TypeInfo* i64_type = create_type_info(TYPE_LONG);
                LLVMValue* n_i64 = cast_to_type(n, i64_type);
                emit_instruction("%%%s = bitcast %s %s to i8*", s1_cast, llvm_type_to_string(s1->llvm_type), format_operand(s1));
                emit_instruction("%%%s = bitcast %s %s to i8*", s2_cast, llvm_type_to_string(s2->llvm_type), format_operand(s2));
                emit_instruction("%%%s = call i32 @memcmp(i8* %%%s, i8* %%%s, i64 %s)", result, s1_cast, s2_cast, format_operand(n_i64));
                free(args);
                return create_llvm_value(LLVM_VALUE_REGISTER, result, create_type_info(TYPE_INT));
            }

            char* res_reg = (ret_type->base_type == TYPE_VOID && ret_type->pointer_level == 0) ? NULL : get_next_reg();
            char* ret_type_str = llvm_type_to_string(ret_type);
            char* params_str = (char*)arena_alloc(g_compiler_arena, 512); params_str[0] = '\0';
            for (int i = 0; i < arg_count; i++) { if (i > 0) strcat(params_str, ", "); if (!args[i]) strcat(params_str, "i32"); else strcat(params_str, llvm_type_to_string(args[i]->llvm_type)); }
            fprintf(g_ctx.output, "  %s%s%s call %s ", res_reg ? "%" : "", res_reg ? res_reg : "", res_reg ? " =" : "", ret_type_str);

            if (func_type && func_type->is_variadic) {
                fprintf(g_ctx.output, "(");
                ASTNode* p = func_type->parameters;
                while (p) {
                    if (p->type == AST_VARIABLE_DECL) fprintf(g_ctx.output, "%s", llvm_type_to_string(p->data.variable_decl.type));
                    else fprintf(g_ctx.output, "i32");
                    fprintf(g_ctx.output, ", "); p = p->next;
                }
                fprintf(g_ctx.output, "...) ");
            } else if (func_type) {
                /* Use full signature for non-variadic calls too for better compatibility */
                fprintf(g_ctx.output, "(%s) ", params_str);
            } else {
                if (!func_val && strcmp(func_name, "printf") == 0) fprintf(g_ctx.output, "(i8*, ...) ");
                else if (!func_val && strcmp(func_name, "fprintf") == 0) fprintf(g_ctx.output, "(i8*, i8*, ...) ");
                else fprintf(g_ctx.output, "(%s) ", params_str);
            }

            if (func_val) fprintf(g_ctx.output, "%s", format_operand(func_val));
            else fprintf(g_ctx.output, "@%s", func_name);

            fprintf(g_ctx.output, "(");
            for (int i = 0; i < arg_count; i++) { if (!args[i]) { fprintf(g_ctx.output, "i32 0%s", (i < arg_count - 1) ? ", " : ""); continue; } fprintf(g_ctx.output, "%s %s%s", llvm_type_to_string(args[i]->llvm_type), format_operand(args[i]), (i < arg_count - 1) ? ", " : ""); }
            fprintf(g_ctx.output, ")\n"); free(args); if (res_reg) return create_llvm_value(LLVM_VALUE_REGISTER, res_reg, ret_type); else return NULL;
        }
        case AST_BINARY_OP: {
            if (expr->data.binary_op.op == OP_AND || expr->data.binary_op.op == OP_OR) {
                int is_and = (expr->data.binary_op.op == OP_AND);
                char* right_label = get_next_label(is_and ? "and_right" : "or_right");
                char* end_label = get_next_label(is_and ? "and_end" : "or_end");
                char* res_name = (char*)arena_alloc(g_compiler_arena, 32);
                sprintf(res_name, "logic_res.%d", g_ctx.next_reg_id++);

                emit_instruction("%%%s = alloca i32", res_name);
                emit_instruction("store i32 %d, i32* %%%s", is_and ? 0 : 1, res_name);

                LLVMValue* left = gen_expression(expr->data.binary_op.left);
                if (!left) return NULL;
                char* left_cond = get_next_reg();
                emit_instruction("%%%s = icmp ne %s %s, %s", left_cond, llvm_type_to_string(left->llvm_type), format_operand(left), (left->llvm_type->pointer_level > 0) ? "null" : "0");

                if (is_and) emit_instruction("br i1 %%%s, label %%%s, label %%%s", left_cond, right_label, end_label);
                else emit_instruction("br i1 %%%s, label %%%s, label %%%s", left_cond, end_label, right_label);

                fprintf(g_ctx.output, "%s:\n", right_label);
                LLVMValue* right = gen_expression(expr->data.binary_op.right);
                char* right_cond = NULL;
                if (right) {
                    right_cond = get_next_reg();
                    emit_instruction("%%%s = icmp ne %s %s, %s", right_cond, llvm_type_to_string(right->llvm_type), format_operand(right), (right->llvm_type->pointer_level > 0) ? "null" : "0");
                    char* right_res = get_next_reg();
                    emit_instruction("%%%s = zext i1 %%%s to i32", right_res, right_cond);
                    emit_instruction("store i32 %%%s, i32* %%%s", right_res, res_name);
                } else {
                    /* Right side evaluation failed - set result to 0 */
                    emit_instruction("store i32 0, i32* %%%s", res_name);
                }
                emit_instruction("br label %%%s", end_label);

                fprintf(g_ctx.output, "%s:\n", end_label);
                if (!right) return NULL;
                char* final_res = get_next_reg();
                emit_instruction("%%%s = load i32, i32* %%%s", final_res, res_name);
                return create_llvm_value(LLVM_VALUE_REGISTER, final_res, create_type_info(TYPE_INT));
            }
            LLVMValue* left = NULL; if (expr->data.binary_op.op != OP_ASSIGN && expr->data.binary_op.op < OP_ASSIGN) left = gen_expression(expr->data.binary_op.left);
            LLVMValue* right = gen_expression(expr->data.binary_op.right);
            if (expr->data.binary_op.op != OP_ASSIGN && !left) return NULL; if (!right) return NULL;
            /* Type promotion: promote smaller types to larger type for binary ops */
            if (left && left->llvm_type->pointer_level == 0 && right->llvm_type->pointer_level == 0) {
                int left_size = get_type_size(left->llvm_type);
                int right_size = get_type_size(right->llvm_type);
                if (left_size > right_size) {
                    right = cast_to_type(right, left->llvm_type);
                } else if (right_size > left_size) {
                    left = cast_to_type(left, right->llvm_type);
                }
            }
            const char* op_name = NULL; const char* icmp_cond = NULL;
            switch (expr->data.binary_op.op) {
                case OP_ADD: op_name = "add"; break;
                case OP_SUB: op_name = "sub"; break;
                case OP_MUL: op_name = "mul"; break;
                case OP_DIV: op_name = "sdiv"; break;
                case OP_MOD: op_name = "srem"; break;
                case OP_BITAND: op_name = "and"; break;
                case OP_BITOR:  op_name = "or"; break;
                case OP_XOR:    op_name = "xor"; break;
                case OP_LSHIFT: op_name = "shl"; break;
                case OP_RSHIFT: op_name = "ashr"; break;
                case OP_LT: icmp_cond = "slt"; break;
                case OP_GT: icmp_cond = "sgt"; break;
                case OP_LE: icmp_cond = "sle"; break;
                case OP_GE: icmp_cond = "sge"; break;
                case OP_EQ: icmp_cond = "eq"; break;
                case OP_NE: icmp_cond = "ne"; break;
                case OP_ASSIGN: {
                    LLVMValue* target_addr = gen_address(expr->data.binary_op.left); if (!target_addr) return NULL;
                    TypeInfo* target_type = duplicate_type_info(target_addr->llvm_type); target_type->pointer_level--;



                    LLVMValue* promoted_right = cast_to_type(right, target_type); char* t_str = llvm_type_to_string(target_type);
                    emit_instruction("store %s %s, %s* %s", t_str, format_operand(promoted_right), t_str, format_operand(target_addr));
                    return promoted_right;
                }
                default: return NULL;
            }
            char* reg = get_next_reg(); char* type_str = llvm_type_to_string(left->llvm_type);
            if (icmp_cond) { emit_instruction("%%%s = icmp %s %s %s, %s", reg, icmp_cond, type_str, format_operand(left), format_operand(right)); char* zext_reg = get_next_reg(); emit_instruction("%%%s = zext i1 %%%s to i32", zext_reg, reg); return create_llvm_value(LLVM_VALUE_REGISTER, zext_reg, create_type_info(TYPE_INT)); }
            else if (left->llvm_type->pointer_level > 0 && right->llvm_type->pointer_level > 0 && strcmp(op_name, "sub") == 0) {
                /* Pointer - Pointer subtraction: convert both to integers first */
                char* left_int = get_next_reg(); char* right_int = get_next_reg();
                emit_instruction("%%%s = ptrtoint %s %s to i64", left_int, type_str, format_operand(left));
                emit_instruction("%%%s = ptrtoint %s %s to i64", right_int, type_str, format_operand(right));
                emit_instruction("%%%s = sub i64 %%%s, %%%s", reg, left_int, right_int);
                return create_llvm_value(LLVM_VALUE_REGISTER, reg, create_type_info(TYPE_LONG));
            }
            else if (left->llvm_type->pointer_level > 0 && right->llvm_type->pointer_level == 0 && strcmp(op_name, "add") == 0) {
                /* Pointer + Integer: use getelementptr */
                fprintf(stderr, "DEBUG: Pointer+Integer Add. Left: %s (ptr=%d), Right: %s (ptr=%d)\n", llvm_type_to_string(left->llvm_type), left->llvm_type->pointer_level, llvm_type_to_string(right->llvm_type), right->llvm_type->pointer_level);
                TypeInfo* pointed_to = duplicate_type_info(left->llvm_type); pointed_to->pointer_level--;
                char* elem_type_str = llvm_type_to_string(pointed_to);
                emit_instruction("%%%s = getelementptr %s, %s %s, %s %s", reg, elem_type_str, type_str, format_operand(left), llvm_type_to_string(right->llvm_type), format_operand(right));
                /* Ensure we return a correct pointer type by duplicating */
                TypeInfo* ret_type = duplicate_type_info(left->llvm_type);
                fprintf(stderr, "DEBUG: Returning type %s (ptr=%d)\n", llvm_type_to_string(ret_type), ret_type->pointer_level);
                return create_llvm_value(LLVM_VALUE_REGISTER, reg, ret_type);
            }
            else if (left->llvm_type->pointer_level > 0 && right->llvm_type->pointer_level == 0 && strcmp(op_name, "sub") == 0) {
                /* Pointer - Integer: negate and use getelementptr */
                TypeInfo* pointed_to = duplicate_type_info(left->llvm_type); pointed_to->pointer_level--;
                char* elem_type_str = llvm_type_to_string(pointed_to);
                char* neg_reg = get_next_reg();
                emit_instruction("%%%s = sub %s 0, %s", neg_reg, llvm_type_to_string(right->llvm_type), format_operand(right));
                emit_instruction("%%%s = getelementptr %s, %s %s, %s %%%s", reg, elem_type_str, type_str, format_operand(left), llvm_type_to_string(right->llvm_type), neg_reg);
                return create_llvm_value(LLVM_VALUE_REGISTER, reg, left->llvm_type);
            }
            else {
                emit_instruction("%%%s = %s %s %s, %s", reg, op_name, type_str, format_operand(left), format_operand(right));
                TypeInfo* result_type = left->llvm_type;
                /* If expression has a specific type (from declaration), use that type and cast result */
                if (expr->data_type) {
                    result_type = expr->data_type;
                    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, reg, left->llvm_type);
                    result = cast_to_type(result, expr->data_type);
                    return result;
                }
                return create_llvm_value(LLVM_VALUE_REGISTER, reg, result_type);
            }
        }
        case AST_UNARY_OP: {
            LLVMValue* operand = gen_expression(expr->data.unary_op.operand); if (!operand) return NULL;
            if (expr->data.unary_op.op == UOP_MINUS) {
                char* reg = get_next_reg();
                emit_instruction("%%%s = sub %s 0, %s", reg, llvm_type_to_string(operand->llvm_type), format_operand(operand));
                return create_llvm_value(LLVM_VALUE_REGISTER, reg, operand->llvm_type);
            }
            if (expr->data.unary_op.op == UOP_BITNOT) { char* reg = get_next_reg(); emit_instruction("%%%s = xor %s %s, -1", reg, llvm_type_to_string(operand->llvm_type), format_operand(operand)); return create_llvm_value(LLVM_VALUE_REGISTER, reg, operand->llvm_type); }
            if (expr->data.unary_op.op == UOP_ADDR) return gen_address(expr->data.unary_op.operand);
            if (expr->data.unary_op.op == UOP_DEREF) { char* reg = get_next_reg(); TypeInfo* res_type = duplicate_type_info(operand->llvm_type); res_type->pointer_level--; emit_instruction("%%%s = load %s, %s* %s", reg, llvm_type_to_string(res_type), llvm_type_to_string(res_type), format_operand(operand)); return create_llvm_value(LLVM_VALUE_REGISTER, reg, res_type); }
            if (expr->data.unary_op.op == UOP_NOT) { char* reg = get_next_reg(); emit_instruction("%%%s = icmp eq %s %s, %s", reg, llvm_type_to_string(operand->llvm_type), format_operand(operand), (operand->llvm_type->pointer_level > 0) ? "null" : "0"); char* zext_reg = get_next_reg(); emit_instruction("%%%s = zext i1 %%%s to i32", zext_reg, reg); return create_llvm_value(LLVM_VALUE_REGISTER, zext_reg, create_type_info(TYPE_INT)); }
            return operand;
        }
        case AST_ARRAY_ACCESS: {
            LLVMValue* addr = gen_address(expr); if (!addr) return NULL;
            TypeInfo* elem_type = duplicate_type_info(addr->llvm_type); elem_type->pointer_level--;
            char* reg = get_next_reg(); char* type_str = llvm_type_to_string(elem_type);
            emit_instruction("%%%s = load %s, %s* %s", reg, type_str, type_str, format_operand(addr)); return create_llvm_value(LLVM_VALUE_REGISTER, reg, elem_type);
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
            /* For arrays, return pointer to element type, not pointer to array */
            if (sym->type->array_size > 0) {
                TypeInfo* elem_type = duplicate_type_info(sym->type->return_type);
                return create_llvm_value(sym->is_global ? LLVM_VALUE_GLOBAL : LLVM_VALUE_REGISTER, sym->name, create_pointer_type(elem_type));
            }
            return create_llvm_value(sym->is_global ? LLVM_VALUE_GLOBAL : LLVM_VALUE_REGISTER, sym->name, create_pointer_type(sym->type));
        }
        case AST_ARRAY_ACCESS: {
            LLVMValue* base = gen_expression(expr->data.array_access.array); LLVMValue* index = gen_expression(expr->data.array_access.index);
            if (!base || !index || !base->llvm_type) return NULL;
            char* res_reg = get_next_reg(); TypeInfo* pointed_to = duplicate_type_info(base->llvm_type); pointed_to->pointer_level--;
            /* For getelementptr, the result type is the same as the first operand type */
            emit_instruction("%%%s = getelementptr %s, %s %s, %s %s", res_reg, llvm_type_to_string(pointed_to), llvm_type_to_string(base->llvm_type), format_operand(base), llvm_type_to_string(index->llvm_type), format_operand(index));
            return create_llvm_value(LLVM_VALUE_REGISTER, res_reg, base->llvm_type);
        }
        case AST_MEMBER_ACCESS: {
            ASTNode* object = expr->data.member_access.object; char* member_name = expr->data.member_access.member; int is_ptr = expr->data.member_access.is_pointer_access;
            LLVMValue* base_ptr = NULL; TypeInfo* struct_type = NULL;
            if (is_ptr) { base_ptr = gen_expression(object); if (!base_ptr || !base_ptr->llvm_type) return NULL; struct_type = duplicate_type_info(base_ptr->llvm_type); struct_type->pointer_level--; }
            else { base_ptr = gen_address(object); if (!base_ptr || !base_ptr->llvm_type) return NULL; struct_type = duplicate_type_info(base_ptr->llvm_type); struct_type->pointer_level--; }
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
            Symbol* sym = create_symbol(stmt->data.variable_decl.name, stmt->data.variable_decl.type);
            sym->original_name = sym->name; /* save original name for lookup */
            symbol_add_local(sym);
            char* type_str = llvm_type_to_string(sym->type); char* unique_name = (char*)arena_alloc(g_compiler_arena, strlen(sym->name) + 16);
            sprintf(unique_name, "%s.%d", sym->name, g_ctx.next_reg_id++); sym->name = unique_name;
            emit_instruction("%%%s = alloca %s", sym->name, type_str);
            if (stmt->data.variable_decl.initializer) {
                LLVMValue* init = gen_expression(stmt->data.variable_decl.initializer);
                if (init) { LLVMValue* casted = cast_to_type(init, sym->type); emit_instruction("store %s %s, %s* %%%s", type_str, format_operand(casted), type_str, sym->name); }
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
            char* old_break = g_ctx.current_break_label; char* old_cont = g_ctx.current_continue_label;
            g_ctx.current_break_label = end_label; g_ctx.current_continue_label = cond_label;
            emit_instruction("br label %%%s", cond_label); fprintf(g_ctx.output, "%s:\n", cond_label);
            ASTNode* cond_node = stmt->data.while_stmt.condition; if (cond_node && cond_node->type == AST_EXPRESSION_STMT) cond_node = cond_node->data.return_stmt.expression;
            if (cond_node) {
                LLVMValue* cond = gen_expression(cond_node);
                if (cond) { char* cond_reg = get_next_reg(); emit_instruction("%%%s = icmp ne %s %s, %s", cond_reg, llvm_type_to_string(cond->llvm_type), format_operand(cond), (cond->llvm_type->pointer_level > 0) ? "null" : "0"); emit_instruction("br i1 %%%s, label %%%s, label %%%s", cond_reg, body_label, end_label); }
                else emit_instruction("br label %%%s", body_label);
            } else emit_instruction("br label %%%s", body_label);
            fprintf(g_ctx.output, "%s:\n", body_label); gen_statement(stmt->data.while_stmt.body); emit_instruction("br label %%%s", cond_label);
            fprintf(g_ctx.output, "%s:\n", end_label); g_ctx.current_break_label = old_break; g_ctx.current_continue_label = old_cont; break;
        }
        case AST_DO_WHILE_STMT: {
            char* body_label = get_next_label("do_body"); char* cond_label = get_next_label("do_cond"); char* end_label = get_next_label("do_end");
            char* old_break = g_ctx.current_break_label; char* old_cont = g_ctx.current_continue_label;
            g_ctx.current_break_label = end_label; g_ctx.current_continue_label = cond_label;
            emit_instruction("br label %%%s", body_label); fprintf(g_ctx.output, "%s:\n", body_label);
            gen_statement(stmt->data.while_stmt.body); emit_instruction("br label %%%s", cond_label);
            fprintf(g_ctx.output, "%s:\n", cond_label);
            LLVMValue* cond = gen_expression(stmt->data.while_stmt.condition);
            if (cond) { char* cond_reg = get_next_reg(); emit_instruction("%%%s = icmp ne %s %s, %s", cond_reg, llvm_type_to_string(cond->llvm_type), format_operand(cond), (cond->llvm_type->pointer_level > 0) ? "null" : "0"); emit_instruction("br i1 %%%s, label %%%s, label %%%s", cond_reg, body_label, end_label); }
            else emit_instruction("br label %%%s", body_label);
            fprintf(g_ctx.output, "%s:\n", end_label); g_ctx.current_break_label = old_break; g_ctx.current_continue_label = old_cont; break;
        }
        case AST_FOR_STMT: {
            char* cond_label = get_next_label("for_cond"); char* body_label = get_next_label("for_body"); char* incr_label = get_next_label("for_incr"); char* end_label = get_next_label("for_end");
            char* old_break = g_ctx.current_break_label; char* old_cont = g_ctx.current_continue_label;
            g_ctx.current_break_label = end_label; g_ctx.current_continue_label = incr_label;
            gen_statement(stmt->data.for_stmt.init); emit_instruction("br label %%%s", cond_label); fprintf(g_ctx.output, "%s:\n", cond_label);
            ASTNode* cond_node = stmt->data.for_stmt.condition; if (cond_node && cond_node->type == AST_EXPRESSION_STMT) cond_node = cond_node->data.return_stmt.expression;
            if (cond_node) {
                LLVMValue* cond = gen_expression(cond_node);
                if (cond) { char* cond_reg = get_next_reg(); emit_instruction("%%%s = icmp ne %s %s, %s", cond_reg, llvm_type_to_string(cond->llvm_type), format_operand(cond), (cond->llvm_type->pointer_level > 0) ? "null" : "0"); emit_instruction("br i1 %%%s, label %%%s, label %%%s", cond_reg, body_label, end_label); }
                else emit_instruction("br label %%%s", body_label);
            } else emit_instruction("br label %%%s", body_label);
            fprintf(g_ctx.output, "%s:\n", body_label); gen_statement(stmt->data.for_stmt.body); emit_instruction("br label %%%s", incr_label);
            fprintf(g_ctx.output, "%s:\n", incr_label); if (stmt->data.for_stmt.update) { ASTNode* u = stmt->data.for_stmt.update; if (u->type == AST_EXPRESSION_STMT) u = u->data.return_stmt.expression; gen_expression(u); }
            emit_instruction("br label %%%s", cond_label); fprintf(g_ctx.output, "%s:\n", end_label); g_ctx.current_break_label = old_break; g_ctx.current_continue_label = old_cont; break;
        }
        case AST_SWITCH_STMT: {
            LLVMValue* cond = gen_expression(stmt->data.switch_stmt.expression); if (!cond) break;
            char* end_label = get_next_label("switch_end"); char* old_break = g_ctx.current_break_label; g_ctx.current_break_label = end_label;
            CaseNode* cases = collect_cases(stmt->data.switch_stmt.body, NULL);
            char* default_label = end_label; CaseNode* c = cases; while (c) { if (c->is_default) default_label = c->label; c = c->next; }
            c = cases;
            while (c) {
                if (!c->is_default) {
                    char* next_cmp = get_next_label("switch_next"); char* cmp_reg = get_next_reg();
                    emit_instruction("%%%s = icmp eq %s %s, %d", cmp_reg, llvm_type_to_string(cond->llvm_type), format_operand(cond), c->value);
                    emit_instruction("br i1 %%%s, label %%%s, label %%%s", cmp_reg, c->label, next_cmp);
                    fprintf(g_ctx.output, "%s:\n", next_cmp);
                }
                c = c->next;
            }
            emit_instruction("br label %%%s", default_label);
            gen_statement(stmt->data.switch_stmt.body); emit_instruction("br label %%%s", end_label);
            fprintf(g_ctx.output, "%s:\n", end_label); g_ctx.current_break_label = old_break; break;
        }
        case AST_CASE_STMT:
        case AST_DEFAULT_STMT: {
            if (stmt->data.case_stmt.label) {
                emit_instruction("br label %%%s", stmt->data.case_stmt.label);
                fprintf(g_ctx.output, "%s:\n", stmt->data.case_stmt.label);
            }
            gen_statement(stmt->data.case_stmt.statement); break;
        }
        case AST_BREAK_STMT: { if (g_ctx.current_break_label) emit_instruction("br label %%%s", g_ctx.current_break_label); break; }
        case AST_CONTINUE_STMT: { if (g_ctx.current_continue_label) emit_instruction("br label %%%s", g_ctx.current_continue_label); break; }
        case AST_RETURN_STMT: {
            LLVMValue* val = gen_expression(stmt->data.return_stmt.expression);
            if (val) { LLVMValue* casted = cast_to_type(val, g_ctx.current_function_return_type); emit_instruction("ret %s %s", llvm_type_to_string(g_ctx.current_function_return_type), format_operand(casted)); }
            else emit_instruction("ret void"); break;
        }
        case AST_GOTO_STMT: {
            if (stmt->data.identifier.name) {
                emit_instruction("br label %%%s", get_user_label(stmt->data.identifier.name));
            }
            break;
        }
        case AST_LABEL_STMT: {
            if (stmt->data.identifier.name) {
                char* label = get_user_label(stmt->data.identifier.name);
                emit_instruction("br label %%%s", label); fprintf(g_ctx.output, "%s:\n", label);
            }
            break;
        }
        case AST_EXPRESSION_STMT: { gen_expression(stmt->data.return_stmt.expression); break; }
        default: break;
    }
}

void codegen_run(ASTNode* ast) {
    if (!ast) return;

    /* Mark all typedef symbols as emitted to prevent them being generated as globals */
    Symbol* s = g_global_symbols;
    while (s) {
        if (s->type && s->type->storage_class == STORAGE_TYPEDEF) {
            s->is_emitted = 1;
        }
        s = s->next;
    }

    fprintf(g_ctx.output, "; Generated LLVM IR\ntarget triple = \"arm64-apple-darwin\"\n\n");

    /* Pre-pass: Emit hardcoded intrinsics that aren't in symbols */
    fprintf(g_ctx.output, "declare void @llvm.va_start(i8*)\n");
    fprintf(g_ctx.output, "declare void @llvm.va_end(i8*)\n");
    fprintf(g_ctx.output, "declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1 immarg)\n");
    fprintf(g_ctx.output, "declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg)\n\n");

    /* Pass 1: Collect/Update all global symbols from AST */
    ASTNode* curr = ast;
    while (curr) {
        if (curr->type == AST_VARIABLE_DECL) {
            Symbol* existing = symbol_lookup(curr->data.variable_decl.name);
            if (existing && existing->is_global) {
                if (existing->type->storage_class == STORAGE_EXTERN &&
                    curr->data.variable_decl.type->storage_class != STORAGE_EXTERN) {
                    existing->type = curr->data.variable_decl.type;
                }
            } else {
                Symbol* sym = create_symbol(curr->data.variable_decl.name, curr->data.variable_decl.type);
                symbol_add_global(sym);
            }
        } else if (curr->type == AST_FUNCTION_DEF || curr->type == AST_FUNCTION_DECL) {
            char* name = (curr->type == AST_FUNCTION_DEF) ? curr->data.function_def.name : curr->data.function_def.name;
            TypeInfo* type = (curr->type == AST_FUNCTION_DEF) ?
                create_function_type(curr->data.function_def.return_type, curr->data.function_def.parameters, curr->data.function_def.is_variadic) :
                create_function_type(curr->data.function_def.return_type, curr->data.function_def.parameters, curr->data.function_def.is_variadic);

            Symbol* existing = symbol_lookup(name);
            if (!existing || !existing->is_global) {
                Symbol* sym = create_symbol(name, type);
                sym->is_global = 1;
                symbol_add_global(sym);
            } else {
                /* Update existing function symbol if we have parameters now and didn't before */
                if (existing->type->base_type == TYPE_FUNCTION && !existing->type->parameters && type->parameters) {
                    existing->type->parameters = type->parameters;
                }
            }
        }
        curr = curr->next;
    }

    /* Pass 2: Emit struct definitions from g_all_structs (must be before functions for sizing) */
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

    /* Pass 3: Emit function definitions */
    curr = ast;
    while (curr) {
        if (curr->type == AST_FUNCTION_DEF) {
            Symbol* sym = symbol_lookup(curr->data.function_def.name);
            if (sym) sym->is_emitted = 1;

            symbol_clear_locals(); g_ctx.current_function_return_type = curr->data.function_def.return_type;

            FILE* original_output = g_ctx.output;
            g_ctx.alloca_file = fopen("alloca.tmp", "w+");
            g_ctx.output = fopen("body.tmp", "w+");
            if (!g_ctx.alloca_file || !g_ctx.output) {
                fatal_error("Could not create temporary files for codegen");
            }

            /* Handle parameters: add to symbol table and emit alloca/store */
            ASTNode* param = curr->data.function_def.parameters; int p_idx = 0;
            while (param) {
                Symbol* psym = create_symbol(param->data.variable_decl.name, param->data.variable_decl.type);
                psym->original_name = psym->name;
                symbol_add_local(psym);
                char* p_type = llvm_type_to_string(psym->type);
                char* unique_name = (char*)arena_alloc(g_compiler_arena, strlen(psym->name) + 16);
                sprintf(unique_name, "%s.%d", psym->name, g_ctx.next_reg_id++); psym->name = unique_name;
                emit_instruction("%%%s = alloca %s", psym->name, p_type);
                emit_instruction("store %s %%p%d, %s* %%%s", p_type, p_idx++, p_type, psym->name); param = param->next;
            }

            gen_statement(curr->data.function_def.body);

            fclose(g_ctx.alloca_file);
            fclose(g_ctx.output);
            g_ctx.alloca_file = NULL;
            g_ctx.output = original_output;

            fprintf(g_ctx.output, "define %s @%s(", llvm_type_to_string(curr->data.function_def.return_type), curr->data.function_def.name);
            param = curr->data.function_def.parameters; p_idx = 0;
            while (param) { fprintf(g_ctx.output, "%s %%p%d%s", llvm_type_to_string(param->data.variable_decl.type), p_idx++, param->next ? ", " : ""); param = param->next; }
            fprintf(g_ctx.output, ") {\n");

            /* Copy allocas collected during gen_statement */
            FILE* f = fopen("alloca.tmp", "r");
            if (f) { int c; while ((c = fgetc(f)) != EOF) fputc(c, g_ctx.output); fclose(f); }

            /* Copy body */
            f = fopen("body.tmp", "r");
            if (f) { int c; while ((c = fgetc(f)) != EOF) fputc(c, g_ctx.output); fclose(f); }

            if (curr->data.function_def.return_type->base_type == TYPE_VOID && curr->data.function_def.return_type->pointer_level == 0) emit_instruction("ret void");
            else emit_instruction("ret %s %s", llvm_type_to_string(curr->data.function_def.return_type), (curr->data.function_def.return_type->pointer_level > 0) ? "null" : "0");
            fprintf(g_ctx.output, "}\n\n");
        }
        curr = curr->next;
    }

    /* Pass 4: Emit global variables */
    curr = ast;
    while (curr) {
        if (curr->type == AST_VARIABLE_DECL) {
            Symbol* sym = symbol_lookup(curr->data.variable_decl.name);
            if (sym && sym->is_global && !sym->is_emitted) {
                char* t_str = llvm_type_to_string(sym->type);
                if (sym->type->storage_class == STORAGE_EXTERN) {
                    fprintf(g_ctx.output, "@%s = external global %s\n", sym->name, t_str);
                } else if (sym->type->storage_class == STORAGE_TYPEDEF) {
                    /* Skip typedefs */
                } else if (sym->type->base_type == TYPE_ARRAY || sym->type->base_type == TYPE_STRUCT || sym->type->base_type == TYPE_UNION) {
                    fprintf(g_ctx.output, "@%s = global %s zeroinitializer\n", sym->name, t_str);
                } else {
                    fprintf(g_ctx.output, "@%s = global %s %s\n", sym->name, t_str,
                        (sym->type->pointer_level > 0) ? "null" : "0");
                }
                sym->is_emitted = 1;
            }
        }
        curr = curr->next;
    }

    /* Pass 5: Emit function declarations for remaining function symbols */
    Symbol* g_sym = g_global_symbols;
    while (g_sym) {
        if (g_sym->type && g_sym->type->base_type == TYPE_FUNCTION && !g_sym->is_emitted && strncmp(g_sym->name, "llvm.", 5) != 0) {
            fprintf(g_ctx.output, "declare %s @%s(", llvm_type_to_string(g_sym->type->return_type), g_sym->name);
            ASTNode* p = g_sym->type->parameters;
            while (p) {
                if (p->type == AST_VARIABLE_DECL) fprintf(g_ctx.output, "%s", llvm_type_to_string(p->data.variable_decl.type));
                else fprintf(g_ctx.output, "i32");
                if (p->next || g_sym->type->is_variadic) fprintf(g_ctx.output, ", ");
                p = p->next;
            }
            if (g_sym->type->is_variadic) fprintf(g_ctx.output, "...");
            fprintf(g_ctx.output, ")\n");
            g_sym->is_emitted = 1;
        }
        g_sym = g_sym->next;
    }

    StringLiteral* sl = g_string_literals;
    while (sl) {
        int already_emitted = 0; StringLiteral* p_sl = g_string_literals;
        while (p_sl != sl) { if (strcmp(p_sl->label, sl->label) == 0) { already_emitted = 1; break; } p_sl = p_sl->next; }
        if (!already_emitted) {
            int true_len = 0; for (int i = 0; i < sl->length; i++) { if (sl->value[i] == '\\') { if (i + 1 < sl->length) { i++; true_len++; } } else true_len++; }
            fprintf(g_ctx.output, "@%s = private unnamed_addr constant [%d x i8] c", sl->label, true_len + 1); fputc(34, g_ctx.output);
            for (int i = 0; i < sl->length; i++) {
                if (sl->value[i] == '\\') {
                    if (i + 1 < sl->length) {
                        if (sl->value[i+1] == 'n') { fputc(92, g_ctx.output); fprintf(g_ctx.output, "0A"); }
                        else if (sl->value[i+1] == 't') { fputc(92, g_ctx.output); fprintf(g_ctx.output, "09"); }
                        else if (sl->value[i+1] == 'r') { fputc(92, g_ctx.output); fprintf(g_ctx.output, "0D"); }
                        else if (sl->value[i+1] == '0') { fputc(92, g_ctx.output); fprintf(g_ctx.output, "00"); }
                        else if (sl->value[i+1] == '\\') { fputc(92, g_ctx.output); fprintf(g_ctx.output, "5C"); }
                        else if (sl->value[i+1] == '"') { fputc(92, g_ctx.output); fprintf(g_ctx.output, "22"); }
                        else fprintf(g_ctx.output, "%c", sl->value[i+1]); i++;
                    }
                } else if (sl->value[i] == '\"') { fputc(92, g_ctx.output); fprintf(g_ctx.output, "22"); }
                else fprintf(g_ctx.output, "%c", sl->value[i]);
            }
            fputc(92, g_ctx.output); fprintf(g_ctx.output, "00"); fputc(34, g_ctx.output); fprintf(g_ctx.output, ", align 1"); fputc(10, g_ctx.output);
        }
        sl = sl->next;
    }
}

void codegen_cleanup(void) {}

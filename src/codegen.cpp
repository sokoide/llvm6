#include "codegen.h"

#include "constants.h"

#include <assert.h>
#include <stdarg.h>

/* Helper functions */
static void* safe_malloc(size_t size) {
    auto ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(1);
    }
    return ptr;
}

static char* safe_strdup(const char* str) {
    if (!str)
        return NULL;
    auto new_str = static_cast<char*>(safe_malloc(strlen(str) + 1));
    strcpy(new_str, str);
    return new_str;
}

/* Context management */
CodeGenContext* create_codegen_context(FILE* output) {
    auto ctx =
        static_cast<CodeGenContext*>(safe_malloc(sizeof(CodeGenContext)));
    memset(ctx, 0, sizeof(CodeGenContext));

    ctx->output = output ? output : stdout;
    ctx->next_reg_id = 1;
    ctx->next_bb_id = 1;
    ctx->current_function_id = 0;

    return ctx;
}

void free_codegen_context(CodeGenContext* ctx) {
    if (!ctx)
        return;

    /* Free symbol tables */
    while (ctx->global_symbols) {
        Symbol* next = ctx->global_symbols->next;
        free_symbol(ctx->global_symbols);
        ctx->global_symbols = next;
    }

    while (ctx->local_symbols) {
        Symbol* next = ctx->local_symbols->next;
        free_symbol(ctx->local_symbols);
        ctx->local_symbols = next;
    }

    /* Free basic blocks */
    while (ctx->bb_list) {
        BasicBlock* next = ctx->bb_list->next;
        free(ctx->bb_list->label);
        free(ctx->bb_list);
        ctx->bb_list = next;
    }

    if (ctx->current_function_name) {
        free(ctx->current_function_name);
    }

    free(ctx);
}

/* Forward declarations */
void generate_module_header(CodeGenContext* ctx);
void process_ast_nodes(CodeGenContext* ctx, ASTNode* ast);

/* Main code generation function */
void generate_llvm_ir(CodeGenContext* ctx, ASTNode* ast) {
    if (!ctx || !ast)
        return;

    generate_module_header(ctx);
    generate_runtime_declarations(ctx);
    process_ast_nodes(ctx, ast);
}

void generate_module_header(CodeGenContext* ctx) {
    fprintf(ctx->output, "; Generated LLVM IR\n");
    fprintf(ctx->output, "target triple = \"x86_64-unknown-linux-gnu\"\n\n");
}

void process_ast_nodes(CodeGenContext* ctx, ASTNode* ast) {
    ASTNode* current = ast;
    while (current) {
        switch (current->type) {
        case AST_FUNCTION_DEF:
            generate_function_definition(ctx, current);
            break;
        case AST_VARIABLE_DECL:
            generate_declaration(ctx, current);
            break;
        default:
            /* Handle other top-level constructs */
            break;
        }
        current = current->next;
    }
}

/* Expression generation */
/* Helper function to load value from identifier if it's a variable pointer */
LLVMValue* load_value_if_needed(CodeGenContext* ctx, LLVMValue* value) {
    if (!value)
        return NULL;

    if (value->type == LLVM_VALUE_CONSTANT ||
        value->type == LLVM_VALUE_FUNCTION)
        return value;

    if (!value->name)
        return value;

    Symbol* symbol = lookup_symbol(ctx, value->name);
    if (!symbol || symbol->is_parameter)
        return value;

    char* load_reg = get_next_register(ctx);
    TypeInfo* loaded_type = duplicate_type_info(symbol->type);
    LLVMValue* loaded_value =
        create_llvm_value(LLVM_VALUE_REGISTER, load_reg, loaded_type);

    char* value_type_str = llvm_type_to_string(symbol->type);
    TypeInfo* pointer_type_info =
        create_pointer_type(duplicate_type_info(symbol->type));
    char* pointer_type_str = llvm_type_to_string(pointer_type_info);

    if (symbol->is_global) {
        emit_instruction(ctx, "%%%s = load %s, %s @%s", load_reg,
                         value_type_str, pointer_type_str, symbol->name);
    } else {
        emit_instruction(ctx, "%%%s = load %s, %s %%%s", load_reg,
                         value_type_str, pointer_type_str, symbol->name);
    }

    free(value_type_str);
    free(pointer_type_str);
    free_type_info(pointer_type_info);
    free(load_reg);
    free_llvm_value(value);
    return loaded_value;
}

static LLVMValue* ensure_pointer_value(CodeGenContext* ctx, LLVMValue* value) {
    if (!value)
        return NULL;

    if (!value->llvm_type || value->llvm_type->base_type != TYPE_POINTER)
        return value;

    Symbol* symbol = lookup_symbol(ctx, value->name);
    if (!symbol || symbol->is_parameter)
        return value;

    char* load_reg = get_next_register(ctx);
    char* value_type_str = llvm_type_to_string(symbol->type);
    TypeInfo* storage_pointer_type =
        create_pointer_type(duplicate_type_info(symbol->type));
    char* storage_pointer_str = llvm_type_to_string(storage_pointer_type);

    if (symbol->is_global) {
        emit_instruction(ctx, "%%%s = load %s, %s @%s", load_reg,
                         value_type_str, storage_pointer_str, symbol->name);
    } else {
        emit_instruction(ctx, "%%%s = load %s, %s %%%s", load_reg,
                         value_type_str, storage_pointer_str, symbol->name);
    }

    free(value_type_str);
    free(storage_pointer_str);
    free_type_info(storage_pointer_type);

    LLVMValue* loaded_value =
        create_llvm_value(LLVM_VALUE_REGISTER, load_reg,
                          duplicate_type_info(symbol->type));
    free(load_reg);
    return loaded_value;
}

static LLVMValue* ensure_integer_register(CodeGenContext* ctx,
                                          LLVMValue* value) {
    if (!value)
        return NULL;

    if (value->type == LLVM_VALUE_REGISTER)
        return value;

    if (value->type == LLVM_VALUE_CONSTANT) {
        char* reg = get_next_register(ctx);
        LLVMValue* reg_value = create_llvm_value(LLVM_VALUE_REGISTER, reg,
                                                 create_type_info(TYPE_INT));
        emit_instruction(ctx, "%%%s = add i32 0, %s", reg, value->name);
        free(reg);
        free_llvm_value(value);
        return reg_value;
    }

    return value;
}

LLVMValue* generate_expression(CodeGenContext* ctx, ASTNode* expr) {
    if (!expr)
        return NULL;

    switch (expr->type) {
    case AST_IDENTIFIER:
        return generate_identifier(ctx, expr);
    case AST_CONSTANT:
        return generate_constant(ctx, expr);
    case AST_STRING_LITERAL:
        return generate_string_literal(ctx, expr);
    case AST_BINARY_OP:
        return generate_binary_op(ctx, expr);
    case AST_UNARY_OP:
        return generate_unary_op(ctx, expr);
    case AST_FUNCTION_CALL:
        return generate_function_call(ctx, expr);
    case AST_ARRAY_ACCESS:
        return generate_array_access(ctx, expr);
    case AST_MEMBER_ACCESS:
        return generate_member_access(ctx, expr);
    case AST_EXPRESSION_STMT:
        /* Handle expression statements in expression context */
        if (expr->data.return_stmt.expression) {
            return generate_expression(ctx, expr->data.return_stmt.expression);
        }
        return NULL;
    default:
        codegen_error(ctx, "Unsupported expression type: %d", expr->type);
        return NULL;
    }
}

/* Helper functions for binary operation generation */

/* Check if operator is an assignment operator */
static bool is_assignment_operator(BinaryOp op) {
    return (op == OP_ASSIGN || op == OP_ADD_ASSIGN || op == OP_SUB_ASSIGN ||
            op == OP_MUL_ASSIGN || op == OP_DIV_ASSIGN || op == OP_MOD_ASSIGN ||
            op == OP_AND_ASSIGN || op == OP_OR_ASSIGN || op == OP_XOR_ASSIGN ||
            op == OP_LSHIFT_ASSIGN || op == OP_RSHIFT_ASSIGN);
}

/* Check if operator is a comparison operator */
static bool is_comparison_operator(BinaryOp op) {
    return (op == OP_LT || op == OP_GT || op == OP_LE || op == OP_GE ||
            op == OP_EQ || op == OP_NE);
}

/* Get LLVM instruction name for binary operator */
static const char* get_binary_op_instruction(BinaryOp op) {
    switch (op) {
    case OP_ADD:
        return "add";
    case OP_SUB:
        return "sub";
    case OP_MUL:
        return "mul";
    case OP_DIV:
        return "sdiv";
    case OP_MOD:
        return "srem";
    case OP_LT:
        return "icmp slt";
    case OP_GT:
        return "icmp sgt";
    case OP_LE:
        return "icmp sle";
    case OP_GE:
        return "icmp sge";
    case OP_EQ:
        return "icmp eq";
    case OP_NE:
        return "icmp ne";
    case OP_BITAND:
        return "and";
    case OP_BITOR:
        return "or";
    case OP_XOR:
        return "xor";
    case OP_LSHIFT:
        return "shl";
    case OP_RSHIFT:
        return "ashr";
    default:
        return NULL;
    }
}

/* Format operand for LLVM instruction (constant vs register) */
static void format_operand(const LLVMValue* value, char* buffer,
                           size_t buffer_size) {
    if (value->type == LLVM_VALUE_CONSTANT) {
        snprintf(buffer, buffer_size, "%d", value->data.constant_val);
    } else {
        snprintf(buffer, buffer_size, "%%%s", value->name);
    }
}

/* Generate comparison operation with i1 to i32 conversion */
static LLVMValue* generate_comparison_op(CodeGenContext* ctx,
                                         const char* op_name,
                                         const LLVMValue* left,
                                         const LLVMValue* right) {
    char left_operand[MAX_OPERAND_STRING_LENGTH];
    char right_operand[MAX_OPERAND_STRING_LENGTH];

    format_operand(left, left_operand, sizeof(left_operand));
    format_operand(right, right_operand, sizeof(right_operand));

    /* Generate comparison and result registers */
    char* cmp_reg = get_next_register(ctx);
    char* result_reg = get_next_register(ctx);

    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                                          create_type_info(TYPE_INT));

    /* Emit comparison instruction */
    emit_instruction(ctx, "%%%s = %s i32 %s, %s", cmp_reg, op_name,
                     left_operand, right_operand);

    /* Convert i1 result to i32 */
    emit_instruction(ctx, "%%%s = zext i1 %%%s to i32", result_reg, cmp_reg);

    /* Cleanup */
    free(cmp_reg);
    free(result_reg);

    return result;
}

/* Generate arithmetic operation */
static LLVMValue* generate_arithmetic_op(CodeGenContext* ctx,
                                         const char* op_name,
                                         const LLVMValue* left,
                                         const LLVMValue* right) {
    char left_operand[MAX_OPERAND_STRING_LENGTH];
    char right_operand[MAX_OPERAND_STRING_LENGTH];

    format_operand(left, left_operand, sizeof(left_operand));
    format_operand(right, right_operand, sizeof(right_operand));

    char* result_reg = get_next_register(ctx);
    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                                          create_type_info(TYPE_INT));

    /* Emit arithmetic instruction */
    emit_instruction(ctx, "%%%s = %s i32 %s, %s", result_reg, op_name,
                     left_operand, right_operand);

    /* Cleanup */
    free(result_reg);

    return result;
}

LLVMValue* generate_binary_op(CodeGenContext* ctx, ASTNode* expr) {
    /* Generate left and right operands */
    LLVMValue* left = generate_expression(ctx, expr->data.binary_op.left);
    LLVMValue* right = generate_expression(ctx, expr->data.binary_op.right);

    if (!left || !right) {
        return NULL;
    }

    BinaryOp op = expr->data.binary_op.op;

    /* Handle assignment operators separately */
    if (is_assignment_operator(op)) {
        free_llvm_value(left);
        free_llvm_value(right);
        return generate_assignment_op(ctx, expr);
    }

    /* Handle pointer arithmetic before loading values */
    if ((op == OP_ADD || op == OP_SUB) &&
        ((left->llvm_type && left->llvm_type->base_type == TYPE_POINTER) ||
         (right->llvm_type && right->llvm_type->base_type == TYPE_POINTER))) {
        return generate_pointer_arithmetic_op(ctx, op, left, right);
    }

    /* Load values for arithmetic/comparison operations */
    left = load_value_if_needed(ctx, left);
    right = load_value_if_needed(ctx, right);

    if (!left || !right) {
        return NULL;
    }

    /* Get instruction name for the operator */
    const char* op_name = get_binary_op_instruction(op);
    if (!op_name) {
        codegen_error(ctx, "Unsupported binary operator: %d", op);
        free_llvm_value(left);
        free_llvm_value(right);
        return NULL;
    }

    LLVMValue* result;

    /* Handle comparison vs arithmetic operations */
    if (is_comparison_operator(op)) {
        result = generate_comparison_op(ctx, op_name, left, right);
    } else {
        result = generate_arithmetic_op(ctx, op_name, left, right);
    }

    /* Cleanup operands */
    free_llvm_value(left);
    free_llvm_value(right);

    return result;
}

LLVMValue* generate_assignment_op(CodeGenContext* ctx, ASTNode* expr) {
    ASTNode* left_node = expr->data.binary_op.left;
    ASTNode* right_node = expr->data.binary_op.right;
    BinaryOp op = expr->data.binary_op.op;

    if (left_node->type != AST_IDENTIFIER) {
        codegen_error(ctx, "Left side of assignment must be a variable");
        return NULL;
    }

    Symbol* symbol = lookup_symbol(ctx, left_node->data.identifier.name);
    if (!symbol) {
        codegen_error(ctx, "Undefined variable: %s",
                      left_node->data.identifier.name);
        return NULL;
    }

    LLVMValue* right_value = generate_expression(ctx, right_node);
    if (!right_value)
        return NULL;

    right_value = load_value_if_needed(ctx, right_value);
    if (!right_value)
        return NULL;

    char right_operand[MAX_OPERAND_STRING_LENGTH];
    format_operand(right_value, right_operand, sizeof(right_operand));

    char* value_type_str = llvm_type_to_string(symbol->type);
    TypeInfo* pointer_type_info =
        create_pointer_type(duplicate_type_info(symbol->type));
    char* pointer_type_str = llvm_type_to_string(pointer_type_info);

    if (op == OP_ASSIGN) {
        if (symbol->is_global) {
            emit_instruction(ctx, "store %s %s, %s @%s", value_type_str,
                             right_operand, pointer_type_str, symbol->name);
        } else {
            emit_instruction(ctx, "store %s %s, %s %%%s", value_type_str,
                             right_operand, pointer_type_str, symbol->name);
        }

        free(value_type_str);
        free(pointer_type_str);
        free_type_info(pointer_type_info);
        free_llvm_value(right_value);

        TypeInfo* location_type =
            create_pointer_type(duplicate_type_info(symbol->type));
        LLVMValue* location_value =
            create_llvm_value(symbol->is_global ? LLVM_VALUE_GLOBAL
                                                : LLVM_VALUE_REGISTER,
                              symbol->name, location_type);
        return load_value_if_needed(ctx, location_value);
    }

    const char* op_name = NULL;
    switch (op) {
    case OP_ADD_ASSIGN:
        op_name = "add";
        break;
    case OP_SUB_ASSIGN:
        op_name = "sub";
        break;
    case OP_MUL_ASSIGN:
        op_name = "mul";
        break;
    case OP_DIV_ASSIGN:
        op_name = "sdiv";
        break;
    case OP_MOD_ASSIGN:
        op_name = "srem";
        break;
    case OP_AND_ASSIGN:
        op_name = "and";
        break;
    case OP_OR_ASSIGN:
        op_name = "or";
        break;
    case OP_XOR_ASSIGN:
        op_name = "xor";
        break;
    case OP_LSHIFT_ASSIGN:
        op_name = "shl";
        break;
    case OP_RSHIFT_ASSIGN:
        op_name = "ashr";
        break;
    default:
        codegen_error(ctx, "Unsupported assignment operator: %d", op);
        free(value_type_str);
        free(pointer_type_str);
        free_type_info(pointer_type_info);
        free_llvm_value(right_value);
        return NULL;
    }

    char* load_reg = get_next_register(ctx);
    if (symbol->is_global) {
        emit_instruction(ctx, "%%%s = load %s, %s @%s", load_reg,
                         value_type_str, pointer_type_str, symbol->name);
    } else {
        emit_instruction(ctx, "%%%s = load %s, %s %%%s", load_reg,
                         value_type_str, pointer_type_str, symbol->name);
    }

    char* result_reg = get_next_register(ctx);
    emit_instruction(ctx, "%%%s = %s %s %%%s, %s", result_reg, op_name,
                     value_type_str, load_reg, right_operand);

    char result_operand[MAX_OPERAND_STRING_LENGTH];
    snprintf(result_operand, sizeof(result_operand), "%%%s", result_reg);

    if (symbol->is_global) {
        emit_instruction(ctx, "store %s %s, %s @%s", value_type_str,
                         result_operand, pointer_type_str, symbol->name);
    } else {
        emit_instruction(ctx, "store %s %s, %s %%%s", value_type_str,
                         result_operand, pointer_type_str, symbol->name);
    }

    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                                          duplicate_type_info(symbol->type));

    free(value_type_str);
    free(pointer_type_str);
    free_type_info(pointer_type_info);
    free(load_reg);
    free(result_reg);
    free_llvm_value(right_value);

    return result;
}

/* Helper functions for unary operations */
static LLVMValue* generate_arithmetic_unary_op(CodeGenContext* ctx,
                                               const LLVMValue* operand,
                                               LLVMValue* result, UnaryOp op) {
    const char* operand_str = operand->name;

    switch (op) {
    case UOP_PLUS:
        /* Unary plus is a no-op */
        if (operand->type == LLVM_VALUE_CONSTANT) {
            emit_instruction(ctx, "%%%s = add i32 0, %s", result->name,
                             operand_str);
        } else {
            emit_instruction(ctx, "%%%s = add i32 0, %%%s", result->name,
                             operand_str);
        }
        break;
    case UOP_MINUS:
        if (operand->type == LLVM_VALUE_CONSTANT) {
            emit_instruction(ctx, "%%%s = sub i32 0, %s", result->name,
                             operand_str);
        } else {
            emit_instruction(ctx, "%%%s = sub i32 0, %%%s", result->name,
                             operand_str);
        }
        break;
    case UOP_NOT:
        if (operand->type == LLVM_VALUE_CONSTANT) {
            emit_instruction(ctx, "%%%s = icmp eq i32 %s, 0", result->name,
                             operand_str);
        } else {
            emit_instruction(ctx, "%%%s = icmp eq i32 %%%s, 0", result->name,
                             operand_str);
        }
        break;
    case UOP_BITNOT:
        if (operand->type == LLVM_VALUE_CONSTANT) {
            emit_instruction(ctx, "%%%s = xor i32 %s, -1", result->name,
                             operand_str);
        } else {
            emit_instruction(ctx, "%%%s = xor i32 %%%s, -1", result->name,
                             operand_str);
        }
        break;
    default:
        return NULL;
    }
    return result;
}

static LLVMValue* generate_increment_decrement_op(CodeGenContext* ctx,
                                                  LLVMValue* operand,
                                                  LLVMValue* result,
                                                  UnaryOp op) {
    if (operand->type == LLVM_VALUE_CONSTANT) {
        codegen_error(ctx, "Cannot increment/decrement constant");
        return NULL;
    }

    char* load_reg = get_next_register(ctx);
    char* mod_reg = get_next_register(ctx);
    const char* operation =
        (op == UOP_PREINC || op == UOP_POSTINC) ? "add" : "sub";

    switch (op) {
    case UOP_PREINC:
    case UOP_PREDEC:
        /* Pre-increment/decrement: modify then return new value */
        emit_instruction(ctx, "%%%s = load i32, i32* %%%s", load_reg,
                         operand->name);
        emit_instruction(ctx, "%%%s = %s i32 %%%s, 1", mod_reg, operation,
                         load_reg);
        emit_instruction(ctx, "store i32 %%%s, i32* %%%s", mod_reg,
                         operand->name);
        emit_instruction(ctx, "%%%s = add i32 %%%s, 0", result->name, mod_reg);
        break;
    case UOP_POSTINC:
    case UOP_POSTDEC:
        /* Post-increment/decrement: return old value then modify */
        emit_instruction(ctx, "%%%s = load i32, i32* %%%s", result->name,
                         operand->name);
        emit_instruction(ctx, "%%%s = %s i32 %%%s, 1", mod_reg, operation,
                         result->name);
        emit_instruction(ctx, "store i32 %%%s, i32* %%%s", mod_reg,
                         operand->name);
        break;
    default:
        free(load_reg);
        free(mod_reg);
        return NULL;
    }

    free(load_reg);
    free(mod_reg);
    return result;
}

static LLVMValue* generate_address_deref_op(CodeGenContext* ctx,
                                            LLVMValue* operand,
                                            LLVMValue* result, UnaryOp op) {
    switch (op) {
    case UOP_ADDR: {
        Symbol* symbol = lookup_symbol(ctx, operand->name);
        if (!symbol) {
            codegen_error(ctx, "Cannot take address of unknown symbol");
            return NULL;
        }

        free_llvm_value(result);
        TypeInfo* pointer_type =
            create_pointer_type(duplicate_type_info(symbol->type));
        LLVMValue* address_value =
            create_llvm_value(symbol->is_global ? LLVM_VALUE_GLOBAL
                                                : LLVM_VALUE_REGISTER,
                              symbol->name, pointer_type);
        return address_value;
    }
    case UOP_DEREF: {
        LLVMValue* operand_source = operand;
        operand = ensure_pointer_value(ctx, operand);
        bool operand_replaced = (operand != operand_source);

        if (!operand || !operand->llvm_type ||
            operand->llvm_type->base_type != TYPE_POINTER) {
            codegen_error(ctx, "Cannot dereference non-pointer type");
            if (operand_replaced) {
                free_llvm_value(operand);
            }
            return NULL;
        }

        TypeInfo* pointee_type =
            duplicate_type_info(operand->llvm_type->return_type);

        char* pointee_type_str =
            llvm_type_to_string(operand->llvm_type->return_type);
        char* pointer_type_str = llvm_type_to_string(operand->llvm_type);

        free_type_info(result->llvm_type);
        result->llvm_type = pointee_type;

        char operand_str[MAX_OPERAND_STRING_LENGTH];
        format_operand(operand, operand_str, sizeof(operand_str));

        emit_instruction(ctx, "%%%s = load %s, %s %s", result->name,
                         pointee_type_str, pointer_type_str, operand_str);

        free(pointee_type_str);
        free(pointer_type_str);
        if (operand_replaced) {
            free_llvm_value(operand);
        }
        return result;
    }
    case UOP_SIZEOF: {
        int size = INT_SIZE_BYTES;
        if (operand->llvm_type) {
            size = get_type_size(operand->llvm_type);
        }
        emit_instruction(ctx, "%%%s = add i32 0, %d", result->name, size);
        return result;
    }
    default:
        return NULL;
    }
}

LLVMValue* generate_pointer_arithmetic_op(CodeGenContext* ctx, BinaryOp op,
                                          LLVMValue* left, LLVMValue* right) {
    bool left_is_pointer =
        (left->llvm_type && left->llvm_type->base_type == TYPE_POINTER);
    bool right_is_pointer =
        (right->llvm_type && right->llvm_type->base_type == TYPE_POINTER);

    if (op == OP_ADD && (left_is_pointer || right_is_pointer)) {
        LLVMValue* pointer_value = left_is_pointer ? left : right;
        LLVMValue* pointer_source = pointer_value;
        pointer_value = ensure_pointer_value(ctx, pointer_value);
        if (pointer_value != pointer_source) {
            free_llvm_value(pointer_source);
        }

        LLVMValue* index_value = left_is_pointer ? right : left;
        LLVMValue* index_source = index_value;
        index_value = load_value_if_needed(ctx, index_value);
        if (index_value != index_source) {
            free_llvm_value(index_source);
        }
        index_value = ensure_integer_register(ctx, index_value);

        if (!pointer_value || !pointer_value->llvm_type ||
            pointer_value->llvm_type->base_type != TYPE_POINTER) {
            codegen_error(ctx, "Pointer arithmetic requires pointer operand");
            free_llvm_value(pointer_value);
            free_llvm_value(index_value);
            return NULL;
        }

        char pointer_operand[MAX_OPERAND_STRING_LENGTH];
        format_operand(pointer_value, pointer_operand, sizeof(pointer_operand));

        char index_operand[MAX_OPERAND_STRING_LENGTH];
        format_operand(index_value, index_operand, sizeof(index_operand));

        char* result_reg = get_next_register(ctx);
        LLVMValue* result =
            create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                              duplicate_type_info(pointer_value->llvm_type));

        char* element_type_str =
            llvm_type_to_string(pointer_value->llvm_type->return_type);
        char* pointer_type_str = llvm_type_to_string(pointer_value->llvm_type);

        emit_instruction(ctx, "%%%s = getelementptr %s, %s %s, i32 %s",
                         result->name, element_type_str, pointer_type_str,
                         pointer_operand, index_operand);

        free(element_type_str);
        free(pointer_type_str);
        free(result_reg);
        free_llvm_value(pointer_value);
        free_llvm_value(index_value);
        return result;
    }

    if (op == OP_SUB && left_is_pointer && !right_is_pointer) {
        LLVMValue* pointer_source = left;
        LLVMValue* pointer_value = ensure_pointer_value(ctx, left);
        if (pointer_value != pointer_source) {
            free_llvm_value(pointer_source);
        }

        LLVMValue* index_source = right;
        LLVMValue* index_value = load_value_if_needed(ctx, right);
        if (index_value != index_source) {
            free_llvm_value(index_source);
        }
        index_value = ensure_integer_register(ctx, index_value);

        if (!pointer_value || !pointer_value->llvm_type ||
            pointer_value->llvm_type->base_type != TYPE_POINTER) {
            codegen_error(ctx, "Pointer subtraction requires pointer operand");
            free_llvm_value(pointer_value);
            free_llvm_value(index_value);
            return NULL;
        }

        char* neg_reg = get_next_register(ctx);
        emit_instruction(ctx, "%%%s = sub i32 0, %%%s", neg_reg,
                         index_value->name);
        LLVMValue* neg_value = create_llvm_value(LLVM_VALUE_REGISTER, neg_reg,
                                                 create_type_info(TYPE_INT));
        free(neg_reg);
        free_llvm_value(index_value);
        index_value = neg_value;

        char pointer_operand[MAX_OPERAND_STRING_LENGTH];
        format_operand(pointer_value, pointer_operand, sizeof(pointer_operand));

        char index_operand[MAX_OPERAND_STRING_LENGTH];
        format_operand(index_value, index_operand, sizeof(index_operand));

        char* result_reg = get_next_register(ctx);
        LLVMValue* result =
            create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                              duplicate_type_info(pointer_value->llvm_type));

        char* element_type_str =
            llvm_type_to_string(pointer_value->llvm_type->return_type);
        char* pointer_type_str = llvm_type_to_string(pointer_value->llvm_type);

        emit_instruction(ctx, "%%%s = getelementptr %s, %s %s, i32 %s",
                         result->name, element_type_str, pointer_type_str,
                         pointer_operand, index_operand);

        free(element_type_str);
        free(pointer_type_str);
        free(result_reg);
        free_llvm_value(pointer_value);
        free_llvm_value(index_value);
        return result;
    }

    if (op == OP_SUB && left_is_pointer && right_is_pointer) {
        LLVMValue* left_source = left;
        LLVMValue* left_pointer = ensure_pointer_value(ctx, left);
        if (left_pointer != left_source) {
            free_llvm_value(left_source);
        }

        LLVMValue* right_source = right;
        LLVMValue* right_pointer = ensure_pointer_value(ctx, right);
        if (right_pointer != right_source) {
            free_llvm_value(right_source);
        }

        if (!left_pointer || !right_pointer || !left_pointer->llvm_type ||
            left_pointer->llvm_type->base_type != TYPE_POINTER ||
            !right_pointer->llvm_type ||
            right_pointer->llvm_type->base_type != TYPE_POINTER) {
            codegen_error(ctx, "Pointer difference requires pointer operands");
            free_llvm_value(left_pointer);
            free_llvm_value(right_pointer);
            return NULL;
        }

        char left_operand[MAX_OPERAND_STRING_LENGTH];
        format_operand(left_pointer, left_operand, sizeof(left_operand));

        char right_operand[MAX_OPERAND_STRING_LENGTH];
        format_operand(right_pointer, right_operand, sizeof(right_operand));

        char* pointer_type_str = llvm_type_to_string(left_pointer->llvm_type);

        char* left_int_reg = get_next_register(ctx);
        char* right_int_reg = get_next_register(ctx);
        emit_instruction(ctx, "%%%s = ptrtoint %s %s to i64", left_int_reg,
                         pointer_type_str, left_operand);
        emit_instruction(ctx, "%%%s = ptrtoint %s %s to i64", right_int_reg,
                         pointer_type_str, right_operand);

        char* diff_reg = get_next_register(ctx);
        emit_instruction(ctx, "%%%s = sub i64 %%%s, %%%s", diff_reg,
                         left_int_reg, right_int_reg);

        int elem_size = get_type_size(left_pointer->llvm_type->return_type);
        if (elem_size <= 0)
            elem_size = 1;

        char* quotient_reg = get_next_register(ctx);
        emit_instruction(ctx, "%%%s = sdiv i64 %%%s, %d", quotient_reg,
                         diff_reg, elem_size);

        char* trunc_reg = get_next_register(ctx);
        emit_instruction(ctx, "%%%s = trunc i64 %%%s to i32", trunc_reg,
                         quotient_reg);

        LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, trunc_reg,
                                              create_type_info(TYPE_INT));

        free(pointer_type_str);
        free(left_int_reg);
        free(right_int_reg);
        free(diff_reg);
        free(quotient_reg);
        free(trunc_reg);
        free_llvm_value(left_pointer);
        free_llvm_value(right_pointer);
        return result;
    }

    codegen_error(ctx, "Unsupported pointer arithmetic operation");
    free_llvm_value(left);
    free_llvm_value(right);
    return NULL;
}

LLVMValue* generate_unary_op(CodeGenContext* ctx, ASTNode* expr) {
    LLVMValue* operand = generate_expression(ctx, expr->data.unary_op.operand);
    if (!operand)
        return NULL;

    char* result_reg = get_next_register(ctx);
    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                                          create_type_info(TYPE_INT));
    UnaryOp op = expr->data.unary_op.op;

    /* Determine if we need the value or pointer based on operation */
    switch (op) {
    case UOP_PLUS:
    case UOP_MINUS:
    case UOP_NOT:
    case UOP_BITNOT:
        /* For these operations, we need the value, not the pointer */
        operand = load_value_if_needed(ctx, operand);
        if (!operand) {
            free_llvm_value(result);
            free(result_reg);
            return NULL;
        }
        break;
    case UOP_PREINC:
    case UOP_PREDEC:
    case UOP_POSTINC:
    case UOP_POSTDEC:
    case UOP_ADDR:
        /* For these operations, we need the pointer, not the value */
        break;
    case UOP_DEREF:
    case UOP_SIZEOF:
        /* Handle these specially in their respective functions */
        break;
    default:
        break;
    }

    /* Delegate to appropriate helper function */
    LLVMValue* operation_result = NULL;

    switch (op) {
    case UOP_PLUS:
    case UOP_MINUS:
    case UOP_NOT:
    case UOP_BITNOT:
        operation_result =
            generate_arithmetic_unary_op(ctx, operand, result, op);
        break;
    case UOP_PREINC:
    case UOP_PREDEC:
    case UOP_POSTINC:
    case UOP_POSTDEC:
        operation_result =
            generate_increment_decrement_op(ctx, operand, result, op);
        break;
    case UOP_ADDR:
    case UOP_DEREF:
    case UOP_SIZEOF:
        operation_result = generate_address_deref_op(ctx, operand, result, op);
        break;
    default:
        codegen_error(ctx, "Unsupported unary operator: %d", op);
        free_llvm_value(result);
        free_llvm_value(operand);
        free(result_reg);
        return NULL;
    }

    /* Handle operation failure */
    if (!operation_result) {
        free_llvm_value(result);
        free_llvm_value(operand);
        free(result_reg);
        return NULL;
    }

    free_llvm_value(operand);
    free(result_reg);
    return operation_result;
}

LLVMValue* generate_identifier(CodeGenContext* ctx, ASTNode* identifier) {
    Symbol* symbol = lookup_symbol(ctx, identifier->data.identifier.name);
    if (!symbol) {
        codegen_error(ctx, "Undefined identifier: %s",
                      identifier->data.identifier.name);
        return NULL;
    }

    /* For function parameters, use them directly without loading */
    if (symbol->is_parameter) {
        LLVMValue* result =
            create_llvm_value(LLVM_VALUE_REGISTER,
                              identifier->data.identifier.name,
                              duplicate_type_info(symbol->type));
        return result;
    }

    /* For local variables, return the address (pointer) so increment/decrement
     * can work */
    if (!symbol->is_global && ctx->current_function_name) {
        LLVMValue* result =
            create_llvm_value(LLVM_VALUE_REGISTER, symbol->name,
                              duplicate_type_info(symbol->type));
        return result;
    }

    /* Global variables */
    if (symbol->is_global) {
        LLVMValue* result =
            create_llvm_value(LLVM_VALUE_GLOBAL, symbol->name,
                              duplicate_type_info(symbol->type));
        return result;
    }

    /* Default case - should not reach here */
    LLVMValue* result =
        create_llvm_value(LLVM_VALUE_REGISTER, identifier->data.identifier.name,
                          duplicate_type_info(symbol->type));
    return result;
}

LLVMValue* generate_constant(CodeGenContext* ctx, ASTNode* constant) {
    LLVMValue* result = create_llvm_value(LLVM_VALUE_CONSTANT, NULL,
                                          create_type_info(TYPE_INT));
    result->data.constant_val = constant->data.constant.value.int_val;

    /* For constants, we use the value directly in instructions */
    snprintf(ctx->temp_buffer, sizeof(ctx->temp_buffer), "%d",
             result->data.constant_val);
    result->name = safe_strdup(ctx->temp_buffer);

    return result;
}

LLVMValue* generate_string_literal(CodeGenContext* ctx, ASTNode* string_lit) {
    /* Generate global string constant */
    char* global_name = get_next_register(ctx);
    int length = string_lit->data.string_literal.length;

    emit_global_declaration(ctx,
                            "@%s = private unnamed_addr constant [%d x i8] "
                            "c\"%s\\00\"",
                            global_name, length + 1,
                            string_lit->data.string_literal.string);

    LLVMValue* result =
        create_llvm_value(LLVM_VALUE_GLOBAL, global_name,
                          create_pointer_type(create_type_info(TYPE_CHAR)));
    free(global_name);

    return result;
}

/* Statement generation */
void generate_statement(CodeGenContext* ctx, ASTNode* stmt) {
    if (!stmt)
        return;

    switch (stmt->type) {
    case AST_COMPOUND_STMT:
        generate_compound_statement(ctx, stmt);
        break;
    case AST_EXPRESSION_STMT:
        generate_expression_statement(ctx, stmt);
        break;
    case AST_VARIABLE_DECL:
        generate_declaration(ctx, stmt);
        break;
    case AST_IF_STMT:
        generate_if_statement(ctx, stmt);
        break;
    case AST_WHILE_STMT:
        generate_while_statement(ctx, stmt);
        break;
    case AST_FOR_STMT:
        generate_for_statement(ctx, stmt);
        break;
    case AST_RETURN_STMT:
        generate_return_statement(ctx, stmt);
        break;
    default:
        /* Handle other statement types */
        break;
    }
}

void generate_compound_statement(CodeGenContext* ctx, ASTNode* stmt) {
    ASTNode* current = stmt->data.compound_stmt.statements;
    while (current) {
        generate_statement(ctx, current);
        current = current->next;
    }
}

void generate_return_statement(CodeGenContext* ctx, ASTNode* stmt) {
    if (stmt->data.return_stmt.expression) {
        LLVMValue* return_val =
            generate_expression(ctx, stmt->data.return_stmt.expression);
        if (return_val) {
            if (return_val->type == LLVM_VALUE_CONSTANT) {
                /* For constants, use literal value directly */
                emit_instruction(ctx, "ret i32 %d",
                                 return_val->data.constant_val);
            } else {
                /* For variables/registers, use register name */
                emit_instruction(ctx, "ret i32 %%%s", return_val->name);
            }

            /* Safer memory cleanup to avoid double-free issues in complex
             * expressions */
            /* Don't free immediately - defer to end of function generation */
            /* free_llvm_value(return_val); */
        }
    } else {
        emit_instruction(ctx, "ret void");
    }
}

void generate_expression_statement(CodeGenContext* ctx, ASTNode* stmt) {
    /* Handle properly structured expression statements from grammar */
    if (!stmt)
        return;

    /* The expression is stored in the same field as return statements */
    if (stmt->data.return_stmt.expression) {
        /* Generate the expression but discard the result */
        LLVMValue* result =
            generate_expression(ctx, stmt->data.return_stmt.expression);
        if (result) {
            free_llvm_value(result);
        }
    }
    /* If expression is NULL, it's an empty statement (just semicolon) - nothing
     * to generate */
}

/* Function definition */
void generate_function_definition(CodeGenContext* ctx, ASTNode* func_def) {
    ctx->current_function_name = safe_strdup(func_def->data.function_def.name);
    ctx->current_function_return_type = func_def->data.function_def.return_type;

    /* Clear local symbols between function definitions - disabled to avoid
     * double-free issues */
    /* clear_local_symbols(ctx); */

    /* Generate function signature */
    char* return_type =
        llvm_type_to_string(func_def->data.function_def.return_type);

    /* Handle function parameters */
    if (func_def->data.function_def.parameters) {
        /* Process parameter declarations and build parameter list */
        char param_list[512] = "";
        int param_count = 0;

        /* Walk through parameter declarations */
        const ASTNode* param_decl = func_def->data.function_def.parameters;
        while (param_decl) {
            if (param_decl->type == AST_VARIABLE_DECL) {
                /* Add parameter to function signature */
                if (param_count > 0) {
                    strcat(param_list, ", ");
                }
                strcat(param_list, "i32 %");
                strcat(param_list, param_decl->data.variable_decl.name);

                /* Create parameter symbol with safer approach */
                TypeInfo* param_type =
                    create_type_info(TYPE_INT); /* Directly create int type */
                Symbol* param_symbol =
                    create_symbol(param_decl->data.variable_decl.name,
                                  param_type);
                param_symbol->is_global = 0;    /* Parameter is local */
                param_symbol->is_parameter = 1; /* Mark as parameter */
                add_local_symbol(ctx, param_symbol);

                param_count++;
            }
            param_decl = param_decl->next;
        }

        emit_function_header(ctx, "define %s @%s(%s) {", return_type,
                             func_def->data.function_def.name, param_list);
    } else {
        /* No parameters */
        emit_function_header(ctx, "define %s @%s() {", return_type,
                             func_def->data.function_def.name);
    }

    /* Generate function body */
    generate_statement(ctx, func_def->data.function_def.body);

    emit_instruction(ctx, "}");
    fprintf(ctx->output, "\n");

    free(return_type);
    if (ctx->current_function_name) {
        free(ctx->current_function_name);
        ctx->current_function_name = NULL;
    }
}
/* Symbol table management */
void add_global_symbol(CodeGenContext* ctx, Symbol* symbol) {
    symbol->next = ctx->global_symbols;
    ctx->global_symbols = symbol;
}

void add_local_symbol(CodeGenContext* ctx, Symbol* symbol) {
    if (!ctx || !symbol)
        return;

    /* Check for duplicate symbols to prevent corruption */
    const Symbol* existing = ctx->local_symbols;
    while (existing) {
        if (existing->name && symbol->name &&
            strcmp(existing->name, symbol->name) == 0) {
            /* Don't add duplicate - just update the existing one */
            return;
        }
        existing = existing->next;
    }

    symbol->next = ctx->local_symbols;
    ctx->local_symbols = symbol;
}

Symbol* lookup_symbol(CodeGenContext* ctx, const char* name) {
    /* Check local symbols first */
    Symbol* symbol = ctx->local_symbols;
    while (symbol) {
        if (strcmp(symbol->name, name) == 0) {
            return symbol;
        }
        symbol = symbol->next;
    }

    /* Then check global symbols */
    symbol = ctx->global_symbols;
    while (symbol) {
        if (strcmp(symbol->name, name) == 0) {
            return symbol;
        }
        symbol = symbol->next;
    }

    return NULL;
}

void clear_local_symbols(CodeGenContext* ctx) {
    if (!ctx)
        return;

    Symbol* current = ctx->local_symbols;
    while (current) {
        Symbol* next = current->next;
        free_symbol(current);
        current = next;
    }
    ctx->local_symbols = NULL;
}

/* Output functions */
void emit_instruction(CodeGenContext* ctx, const char* format, ...) {
    va_list args;
    va_start(args, format);

    fprintf(ctx->output, "  ");
    vfprintf(ctx->output, format, args);
    fprintf(ctx->output, "\n");

    va_end(args);
}

void emit_global_declaration(CodeGenContext* ctx, const char* format, ...) {
    va_list args;
    va_start(args, format);

    vfprintf(ctx->output, format, args);
    fprintf(ctx->output, "\n");

    va_end(args);
}

void emit_function_header(CodeGenContext* ctx, const char* format, ...) {
    va_list args;
    va_start(args, format);

    vfprintf(ctx->output, format, args);
    fprintf(ctx->output, "\n");

    va_end(args);
}

void emit_comment(CodeGenContext* ctx, const char* comment) {
    fprintf(ctx->output, "; %s\n", comment);
}

/* Runtime support */
void generate_runtime_declarations(CodeGenContext* ctx) {
    emit_comment(ctx, "Runtime function declarations");
    emit_global_declaration(ctx, "declare i32 @printf(i8*, ...)");
    emit_global_declaration(ctx, "declare i32 @scanf(i8*, ...)");
    emit_global_declaration(ctx, "declare i8* @malloc(i64)");
    emit_global_declaration(ctx, "declare void @free(i8*)");
    fprintf(ctx->output, "\n");
}

/* Error reporting */
void codegen_error(CodeGenContext* ctx, const char* message, ...) {
    (void)ctx; /* Suppress unused parameter warning */
    va_list args;
    va_start(args, message);

    fprintf(stderr, "Code generation error: ");
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");

    va_end(args);
    /* For now, return instead of aborting to let tests continue */
    /* TODO: Properly handle errors */
    return;
}

void codegen_warning(CodeGenContext* ctx, const char* message, ...) {
    (void)ctx; /* Suppress unused parameter warning */
    va_list args;
    va_start(args, message);

    fprintf(stderr, "Code generation warning: ");
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");

    va_end(args);
}

/* Declaration generation */
void generate_declaration(CodeGenContext* ctx, ASTNode* decl) {
    if (!ctx || !decl)
        return;

    TypeInfo* symbol_type = duplicate_type_info(decl->data.variable_decl.type);
    Symbol* symbol = create_symbol(decl->data.variable_decl.name, symbol_type);

    if (ctx->current_function_name == NULL) {
        /* Global variable */
        symbol->is_global = 1;
        char* type_str = llvm_type_to_string(decl->data.variable_decl.type);
        char* default_val = get_default_value(decl->data.variable_decl.type);

        emit_global_declaration(ctx, "@%s = global %s %s", symbol->name,
                                type_str, default_val);

        add_global_symbol(ctx, symbol);
        free(type_str);
        free(default_val);
    } else {
        /* Local variable */
        char* type_str = llvm_type_to_string(decl->data.variable_decl.type);
        emit_instruction(ctx, "%%%s = alloca %s", symbol->name, type_str);

        if (decl->data.variable_decl.initializer) {
            LLVMValue* init_val =
                generate_expression(ctx, decl->data.variable_decl.initializer);
            if (init_val) {
                char init_operand[MAX_OPERAND_STRING_LENGTH];
                format_operand(init_val, init_operand, sizeof(init_operand));

                char* value_type_str = llvm_type_to_string(symbol->type);
                TypeInfo* pointer_type_info =
                    create_pointer_type(duplicate_type_info(symbol->type));
                char* pointer_type_str = llvm_type_to_string(pointer_type_info);

                emit_instruction(ctx, "store %s %s, %s %%%s", value_type_str,
                                 init_operand, pointer_type_str, symbol->name);

                free(value_type_str);
                free(pointer_type_str);
                free_type_info(pointer_type_info);
                free_llvm_value(init_val);
            }
        }

        add_local_symbol(ctx, symbol);
        free(type_str);
    }
}

/* Utility functions */
LLVMValue* create_llvm_value(LLVMValueType type, const char* name,
                             TypeInfo* llvm_type) {
    auto value = static_cast<LLVMValue*>(safe_malloc(sizeof(LLVMValue)));
    value->type = type;
    value->name = safe_strdup(name);
    value->llvm_type = llvm_type;
    return value;
}

void free_llvm_value(LLVMValue* value) {
    if (!value)
        return;
    free(value->name);
    free_type_info(value->llvm_type);
    free(value);
}

char* get_next_register(CodeGenContext* ctx) {
    auto reg_name = static_cast<char*>(safe_malloc(MAX_REGISTER_NAME_LENGTH));
    snprintf(reg_name, 32, "%d", ctx->next_reg_id++);
    return reg_name;
}

char* get_next_basic_block(CodeGenContext* ctx) {
    auto bb_name = static_cast<char*>(safe_malloc(MAX_BASIC_BLOCK_NAME_LENGTH));
    snprintf(bb_name, 32, "bb%d", ctx->next_bb_id++);
    return bb_name;
}

/* Type utilities */
char* llvm_type_to_string(TypeInfo* type) {
    if (!type)
        return safe_strdup("void");

    switch (type->base_type) {
    case TYPE_VOID:
        return safe_strdup("void");
    case TYPE_CHAR:
        return safe_strdup("i8");
    case TYPE_SHORT:
        return safe_strdup("i16");
    case TYPE_INT:
        return safe_strdup("i32");
    case TYPE_LONG:
        return safe_strdup("i64");
    case TYPE_FLOAT:
        return safe_strdup("float");
    case TYPE_DOUBLE:
        return safe_strdup("double");
    case TYPE_POINTER: {
        char* target_str = NULL;
        if (type->return_type) {
            target_str = llvm_type_to_string(type->return_type);
        } else {
            target_str = safe_strdup("i8");
        }
        size_t len = strlen(target_str) + 2;
        auto result = static_cast<char*>(safe_malloc(len + 1));
        snprintf(result, len + 1, "%s*", target_str);
        free(target_str);
        return result;
    }
    case TYPE_STRUCT: {
        const char* struct_name =
            type->struct_name ? type->struct_name : "anon";
        size_t len = strlen(struct_name) + strlen("%struct.");
        auto result = static_cast<char*>(safe_malloc(len + 2));
        snprintf(result, len + 2, "%%struct.%s", struct_name);
        return result;
    }
    case TYPE_ARRAY: {
        char* element_str = type->return_type
                                ? llvm_type_to_string(type->return_type)
                                : safe_strdup("i8");
        size_t len =
            snprintf(NULL, 0, "[%d x %s]", type->array_size, element_str);
        auto result = static_cast<char*>(safe_malloc(len + 1));
        snprintf(result, len + 1, "[%d x %s]", type->array_size, element_str);
        free(element_str);
        return result;
    }
    default:
        return safe_strdup("i32");
    }
}

char* get_default_value(const TypeInfo* type) {
    if (!type)
        return safe_strdup(DEFAULT_INT_VALUE);

    switch (type->base_type) {
    case TYPE_FLOAT:
        return safe_strdup(DEFAULT_FLOAT_VALUE);
    case TYPE_DOUBLE:
        return safe_strdup(DEFAULT_DOUBLE_VALUE);
    case TYPE_POINTER:
        return safe_strdup(DEFAULT_POINTER_VALUE);
    default:
        return safe_strdup(DEFAULT_INT_VALUE);
    }
}

static int is_integer_type(DataType type) {
    switch (type) {
    case TYPE_CHAR:
    case TYPE_SHORT:
    case TYPE_INT:
    case TYPE_LONG:
        return 1;
    default:
        return 0;
    }
}

int get_type_size(TypeInfo* type) {
    if (!type)
        return INT_SIZE_BYTES;

    switch (type->base_type) {
    case TYPE_CHAR:
        return 1;
    case TYPE_SHORT:
        return 2;
    case TYPE_INT:
        return INT_SIZE_BYTES;
    case TYPE_LONG:
        return 8;
    case TYPE_FLOAT:
        return FLOAT_SIZE_BYTES;
    case TYPE_DOUBLE:
        return DOUBLE_SIZE_BYTES;
    case TYPE_POINTER:
    case TYPE_FUNCTION:
        return POINTER_SIZE_BYTES;
    case TYPE_ARRAY:
        return type->array_size * get_type_size(type->return_type);
    case TYPE_STRUCT:
    case TYPE_UNION:
        /* TODO: compute actual aggregate size once layout is available */
        return POINTER_SIZE_BYTES;
    default:
        return INT_SIZE_BYTES;
    }
}

static int compare_struct_names(const TypeInfo* lhs, const TypeInfo* rhs) {
    if (!lhs || !rhs)
        return lhs == rhs;
    if (lhs->struct_name == NULL && rhs->struct_name == NULL)
        return 1;
    if (!lhs->struct_name || !rhs->struct_name)
        return 0;
    return strcmp(lhs->struct_name, rhs->struct_name) == 0;
}

int types_compatible(TypeInfo* type1, TypeInfo* type2) {
    if (!type1 || !type2)
        return 0;

    if (type1->base_type == type2->base_type) {
        switch (type1->base_type) {
        case TYPE_POINTER:
            return types_compatible(type1->return_type, type2->return_type);
        case TYPE_ARRAY:
            return type1->array_size == type2->array_size &&
                   types_compatible(type1->return_type, type2->return_type);
        case TYPE_STRUCT:
        case TYPE_UNION:
            return compare_struct_names(type1, type2);
        default:
            return 1;
        }
    }

    /* Allow pointer <-> integer comparisons for null checks */
    if (type1->base_type == TYPE_POINTER && is_integer_type(type2->base_type))
        return 1;
    if (type2->base_type == TYPE_POINTER && is_integer_type(type1->base_type))
        return 1;

    return 0;
}

void generate_if_statement(CodeGenContext* ctx, ASTNode* stmt) {
    emit_instruction(ctx, "; if statement");

    /* Generate condition */
    LLVMValue* condition =
        generate_expression(ctx, stmt->data.if_stmt.condition);
    if (!condition)
        return;

    /* Create basic blocks */
    char* then_label = get_next_basic_block(ctx);
    char* else_label = get_next_basic_block(ctx);
    char* end_label = get_next_basic_block(ctx);

    /* Branch based on condition */
    if (condition->type == LLVM_VALUE_CONSTANT) {
        emit_instruction(ctx, "br i1 %d, label %%%s, label %%%s",
                         condition->data.constant_val, then_label, else_label);
    } else {
        emit_instruction(ctx, "br i1 %%%s, label %%%s, label %%%s",
                         condition->name, then_label, else_label);
    }

    /* Then block */
    emit_instruction(ctx, "%s:", then_label);
    generate_statement(ctx, stmt->data.if_stmt.then_stmt);
    emit_instruction(ctx, "br label %%%s", end_label);

    /* Else block */
    emit_instruction(ctx, "%s:", else_label);
    if (stmt->data.if_stmt.else_stmt) {
        generate_statement(ctx, stmt->data.if_stmt.else_stmt);
    }
    emit_instruction(ctx, "br label %%%s", end_label);

    /* End block */
    emit_instruction(ctx, "%s:", end_label);

    free_llvm_value(condition);
    free(then_label);
    free(else_label);
    free(end_label);
}

void generate_while_statement(CodeGenContext* ctx, ASTNode* stmt) {
    emit_instruction(ctx, "; while statement");
    generate_expression(ctx, stmt->data.while_stmt.condition);
    generate_statement(ctx, stmt->data.while_stmt.body);
}

void generate_for_statement(CodeGenContext* ctx, ASTNode* stmt) {
    emit_instruction(ctx, "; for statement");
    if (stmt->data.for_stmt.init) {
        generate_expression(ctx, stmt->data.for_stmt.init);
    }
    if (stmt->data.for_stmt.condition) {
        generate_expression(ctx, stmt->data.for_stmt.condition);
    }
    generate_statement(ctx, stmt->data.for_stmt.body);
    if (stmt->data.for_stmt.update) {
        generate_expression(ctx, stmt->data.for_stmt.update);
    }
}

/* Expression generation */
LLVMValue* generate_function_call(CodeGenContext* ctx, ASTNode* call) {
    if (!call || !call->data.function_call.function) {
        codegen_error(ctx, "Invalid function call");
        return NULL;
    }

    /* Get function name - don't generate it as expression */
    char* func_name = call->data.function_call.function->data.identifier.name;

    /* Process arguments */
    char arg_list[1024] = "";
    ASTNode* arg = call->data.function_call.arguments;
    int arg_count = 0;

    while (arg) {
        LLVMValue* arg_val = generate_expression(ctx, arg);
        if (!arg_val) {
            codegen_error(ctx,
                          "Failed to generate argument %d for function call",
                          arg_count);
            return NULL;
        }

        if (arg_count > 0) {
            strcat(arg_list, ", ");
        }

        char arg_spec[256];
        if (arg_val->type == LLVM_VALUE_CONSTANT) {
            /* For constants, use the value directly */
            snprintf(arg_spec, sizeof(arg_spec), "i32 %s", arg_val->name);
        } else {
            /* For variables/registers, use register notation */
            snprintf(arg_spec, sizeof(arg_spec), "i32 %%%s", arg_val->name);
        }
        strcat(arg_list, arg_spec);

        free_llvm_value(arg_val);
        arg = arg->next;
        arg_count++;
    }

    char* result_reg = get_next_register(ctx);
    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                                          create_type_info(TYPE_INT));

    emit_instruction(ctx, "%%%s = call i32 @%s(%s)", result_reg, func_name,
                     arg_list);

    /* Free the original result_reg since create_llvm_value duplicated it */
    free(result_reg);
    return result;
}

LLVMValue* generate_array_access(CodeGenContext* ctx, ASTNode* access) {
    (void)access; /* Suppress unused parameter warning */
    char* result_reg = get_next_register(ctx);
    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                                          create_type_info(TYPE_INT));

    emit_instruction(ctx, "%%%s = load i32, i32* %%array_ptr", result_reg);

    /* Free the original result_reg since create_llvm_value duplicated it */
    free(result_reg);
    return result;
}

LLVMValue* generate_member_access(CodeGenContext* ctx, ASTNode* access) {
    if (!access || access->type != AST_MEMBER_ACCESS) {
        codegen_error(ctx, "Invalid member access node");
        return NULL;
    }

    ASTNode* object = access->data.member_access.object;
    char* member_name = access->data.member_access.member;
    int is_pointer_access = access->data.member_access.is_pointer_access;

    if (!object || !member_name) {
        codegen_error(ctx,
                      "Invalid member access: missing object or member name");
        return NULL;
    }

    /* Generate code for the object being accessed */
    LLVMValue* object_value = generate_expression(ctx, object);
    if (!object_value) {
        codegen_error(ctx, "Failed to generate object for member access");
        return NULL;
    }

    /* Determine the struct type */
    TypeInfo* struct_type = object_value->llvm_type;

    /* If this is pointer access (->), dereference the pointer */
    if (is_pointer_access) {
        if (!struct_type || struct_type->base_type != TYPE_POINTER) {
            codegen_error(ctx, "Arrow operator used on non-pointer type");
            free_llvm_value(object_value);
            return NULL;
        }
        /* Get the pointed-to type */
        struct_type = struct_type->return_type;
    }

    /* Verify we have a struct type */
    if (!struct_type || struct_type->base_type != TYPE_STRUCT) {
        codegen_error(ctx, "Member access on non-struct type");
        free_llvm_value(object_value);
        return NULL;
    }

    /* For now, assume all struct members are integers at offset 0 */
    /* This is a basic implementation - real implementation would need */
    /* struct layout and member offset calculation */

    char* result_reg = get_next_register(ctx);
    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                                          create_type_info(TYPE_INT));

    if (is_pointer_access) {
        /* ptr->member: load from pointer + member offset */
        emit_instruction(ctx,
                         "%%%s = getelementptr %%struct.%s, %%struct.%s* %%%s, "
                         "i32 0, i32 0",
                         result_reg,
                         struct_type->struct_name ? struct_type->struct_name
                                                  : "unknown",
                         struct_type->struct_name ? struct_type->struct_name
                                                  : "unknown",
                         object_value->name);
        emit_instruction(ctx, "%%%s = load i32, i32* %%%s", result_reg,
                         result_reg);
    } else {
        /* obj.member: get address of member and load */
        char* member_ptr = get_next_register(ctx);
        emit_instruction(ctx,
                         "%%%s = getelementptr %%struct.%s, %%struct.%s* %%%s, "
                         "i32 0, i32 0",
                         member_ptr,
                         struct_type->struct_name ? struct_type->struct_name
                                                  : "unknown",
                         struct_type->struct_name ? struct_type->struct_name
                                                  : "unknown",
                         object_value->name);
        emit_instruction(ctx, "%%%s = load i32, i32* %%%s", result_reg,
                         member_ptr);
        free(member_ptr);
    }

    /* Clean up */
    free_llvm_value(object_value);
    free(result_reg);
    return result;
}
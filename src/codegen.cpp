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

static void emit_all_global_constants(CodeGenContext* ctx);
static void generate_function_declaration(CodeGenContext* ctx, ASTNode* func_decl) {
    /* Check if already declared (e.g. by runtime declarations) */
    if (lookup_symbol(ctx, func_decl->data.function_def.name)) {
        return;
    }

    char* ret_type_str = llvm_type_to_string(func_decl->data.function_def.return_type);
    char params_buf[1024] = "";
    ASTNode* param = func_decl->data.function_def.parameters;

    while (param) {
        char* param_type_str = llvm_type_to_string(param->data.variable_decl.type);
        strcat(params_buf, param_type_str);
        free(param_type_str);
        if (param->next) {
            strcat(params_buf, ", ");
        }
        param = param->next;
    }

    if (func_decl->data.function_def.is_variadic) {
        if (func_decl->data.function_def.parameters) {
            strcat(params_buf, ", ...");
        } else {
            strcat(params_buf, "...");
        }
    }

    emit_global_declaration(ctx, "declare %s @%s(%s)", ret_type_str,
                            func_decl->data.function_def.name, params_buf);
    free(ret_type_str);

    /* Add to global symbol table to allow calls */
    Symbol* symbol = create_symbol(func_decl->data.function_def.name,
                                   duplicate_type_info(func_decl->data.function_def.return_type));
    symbol->is_global = 1;
    add_global_symbol(ctx, symbol);
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

    /* Free global constants */
    while (ctx->global_constants) {
        GlobalConstant* next = ctx->global_constants->next;
        free(ctx->global_constants->declaration);
        free(ctx->global_constants);
        ctx->global_constants = next;
    }

    if (ctx->current_function_name) {
        free(ctx->current_function_name);
    }

    free(ctx);
}

/* Forward declarations */
void generate_module_header(CodeGenContext* ctx);
void process_ast_nodes(CodeGenContext* ctx, ASTNode* ast);
void generate_switch_statement(CodeGenContext* ctx, ASTNode* stmt);

/* Main code generation function */
void generate_llvm_ir(CodeGenContext* ctx, ASTNode* ast) {
    if (!ctx || !ast)
        return;

    generate_module_header(ctx);
    generate_runtime_declarations(ctx);
    process_ast_nodes(ctx, ast);

    /* Emit global constants at the end of the module */
    emit_all_global_constants(ctx);
}

void generate_module_header(CodeGenContext* ctx) {
    fprintf(ctx->output, "; Generated LLVM IR\n");
    fprintf(ctx->output, "target triple = \"arm64-apple-darwin\"\n\n");
}

void process_ast_nodes(CodeGenContext* ctx, ASTNode* ast) {
    ASTNode* current = ast;
    while (current) {
        switch (current->type) {
        case AST_FUNCTION_DEF:
            generate_function_definition(ctx, current);
            break;
        case AST_FUNCTION_DECL:
            generate_function_declaration(ctx, current);
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

    if (!value->is_lvalue)
        return value;

    if (!value->name)
        return value;

    Symbol* symbol = lookup_symbol(ctx, value->name);
    TypeInfo* type = symbol ? symbol->type : value->llvm_type;

    /* Handle array decay: array name returns pointer to first element */
    if (type && type->base_type == TYPE_ARRAY) {
        if (symbol && symbol->is_parameter) return value;

        char* gep_reg = get_next_register(ctx);
        char* array_type_str = llvm_type_to_string(type);
        const char* name = symbol ? symbol->name : value->name;

        if (symbol && symbol->is_global) {
             emit_instruction(ctx, "%%%s = getelementptr %s, %s* @%s, i32 0, i32 0",
                              gep_reg, array_type_str, array_type_str, name);
        } else {
             emit_instruction(ctx, "%%%s = getelementptr %s, %s* %%%s, i32 0, i32 0",
                              gep_reg, array_type_str, array_type_str, name);
        }

        TypeInfo* ptr_type = create_pointer_type(duplicate_type_info(type->return_type));
        LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, gep_reg, ptr_type);

        free(array_type_str);
        return result;
    }

    /* Prevent loading of array pointers (Pointer to Array) */
    if (type && type->base_type == TYPE_POINTER &&
        type->return_type && type->return_type->base_type == TYPE_ARRAY) {
        return value;
    }

    /* Load the value */
    if (symbol && symbol->is_parameter)
        return value;

    char* load_reg = get_next_register(ctx);
    TypeInfo* loaded_type = duplicate_type_info(type);
    LLVMValue* loaded_value =
        create_llvm_value(LLVM_VALUE_REGISTER, load_reg, loaded_type);

    char* value_type_str = llvm_type_to_string(type);
    TypeInfo* pointer_type_info =
        create_pointer_type(duplicate_type_info(type));
    char* pointer_type_str = llvm_type_to_string(pointer_type_info);

    const char* name = symbol ? symbol->name : value->name;

    if (symbol && symbol->is_global) {
        emit_instruction(ctx, "%%%s = load %s, %s @%s", load_reg,
                         value_type_str, pointer_type_str, name);
    } else {
        emit_instruction(ctx, "%%%s = load %s, %s %%%s", load_reg,

                         value_type_str, pointer_type_str, name);
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
    case AST_CONDITIONAL:
        return generate_conditional_op(ctx, expr);
    case AST_CAST:
        return generate_cast(ctx, expr);
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
/* Helper to escape string for LLVM IR */
void escape_string_for_llvm(const char* input, char* output, size_t output_size) {
    size_t j = 0;
    for (size_t i = 0; input[i] != '\0' && j < output_size - 1; ++i) {
        unsigned char c = (unsigned char)input[i];
        if (c == '"' || c == '\\' || c < 32 || c > 126) {
            if (j + 3 >= output_size - 1) break;
            snprintf(output + j, output_size - j, "\\%02X", c);
            j += 3;
        } else {
            output[j++] = (char)c;
        }
    }
    output[j] = '\0';
}

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
    BinaryOp op = expr->data.binary_op.op;

    /* Handle assignment operators separately */
    if (is_assignment_operator(op)) {
        return generate_assignment_op(ctx, expr);
    }

    /* Handle logical AND/OR with short-circuit evaluation */
    if (op == OP_AND || op == OP_OR) {
        char* cond_bb = get_next_basic_block(ctx);
        char* second_bb = get_next_basic_block(ctx);
        char* end_bb = get_next_basic_block(ctx);

        /* First, jump to condition block */
        emit_instruction(ctx, "br label %%%s", cond_bb);

        /* Condition block - evaluate left side */
        emit_basic_block_label(ctx, cond_bb);
        LLVMValue* left = generate_expression(ctx, expr->data.binary_op.left);
        left = load_value_if_needed(ctx, left);
        char left_op[MAX_OPERAND_STRING_LENGTH];
        format_operand(left, left_op, sizeof(left_op));

        char* cond_reg = get_next_register(ctx);
        emit_instruction(ctx, "%%%s = icmp ne i32 %s, 0", cond_reg, left_op);

        if (op == OP_AND) {
            emit_instruction(ctx, "br i1 %%%s, label %%%s, label %%%s", cond_reg,
                             second_bb, end_bb);
        } else {
            emit_instruction(ctx, "br i1 %%%s, label %%%s, label %%%s", cond_reg,
                             end_bb, second_bb);
        }

        emit_basic_block_label(ctx, second_bb);
        LLVMValue* right = generate_expression(ctx, expr->data.binary_op.right);
        right = load_value_if_needed(ctx, right);
        char right_op[MAX_OPERAND_STRING_LENGTH];
        format_operand(right, right_op, sizeof(right_op));

        char* cond_right_reg = get_next_register(ctx);
        emit_instruction(ctx, "%%%s = icmp ne i32 %s, 0", cond_right_reg,
                         right_op);
        emit_instruction(ctx, "br label %%%s", end_bb);

        emit_basic_block_label(ctx, end_bb);
        /* result = phi [ left_bool, cond_bb ], [ right_bool, second_bb ] */
        char* result_reg = get_next_register(ctx);
        char* left_bool_val = (op == OP_AND) ? (char*)"false" : (char*)"true";
        emit_instruction(ctx,
                         "%%%s = phi i1 [ %s, %%%s ], [ %%%s, %%%s ]",
                         result_reg, left_bool_val, cond_bb,
                         cond_right_reg, second_bb);

        char* final_result_reg = get_next_register(ctx);
        emit_instruction(ctx, "%%%s = zext i1 %%%s to i32", final_result_reg,
                         result_reg);

        free_llvm_value(left);
        free_llvm_value(right);
        free(cond_bb);
        free(second_bb);
        free(end_bb);
        free(cond_reg);
        free(cond_right_reg);
        free(result_reg);

        return create_llvm_value(LLVM_VALUE_REGISTER, final_result_reg,
                                 create_type_info(TYPE_INT));
    }

    /* Generate left and right operands */
    LLVMValue* left = generate_expression(ctx, expr->data.binary_op.left);
    LLVMValue* right = generate_expression(ctx, expr->data.binary_op.right);

    if (!left || !right) {
        return NULL;
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

    /* Integer Promotion: Cast types smaller than int to int */
    if (left->llvm_type && get_type_size(left->llvm_type) < 4) {
        char* res_reg = get_next_register(ctx);
        char op_str[MAX_OPERAND_STRING_LENGTH];
        format_operand(left, op_str, sizeof(op_str));
        char* type_str = llvm_type_to_string(left->llvm_type);

        emit_instruction(ctx, "%%%s = sext %s %s to i32", res_reg, type_str, op_str);

        free(type_str);
        TypeInfo* new_type = create_type_info(TYPE_INT);
        /* Preserve is_lvalue? result of cast is rvalue */
        /* Free old left wrapper but keep its register name if it was register?
           format_operand uses name. left->name is strdup'd.
           free_llvm_value frees name. */
        free_llvm_value(left);
        left = create_llvm_value(LLVM_VALUE_REGISTER, res_reg, new_type);
    }

    if (right->llvm_type && get_type_size(right->llvm_type) < 4) {
        char* res_reg = get_next_register(ctx);
        char op_str[MAX_OPERAND_STRING_LENGTH];
        format_operand(right, op_str, sizeof(op_str));
        char* type_str = llvm_type_to_string(right->llvm_type);

        emit_instruction(ctx, "%%%s = sext %s %s to i32", res_reg, type_str, op_str);

        free(type_str);
        TypeInfo* new_type = create_type_info(TYPE_INT);
        free_llvm_value(right);
        right = create_llvm_value(LLVM_VALUE_REGISTER, res_reg, new_type);
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

    /* Handle array access assignment (arr[i] = value) */
    if (left_node->type == AST_ARRAY_ACCESS) {
        if (op != OP_ASSIGN) {
            codegen_error(ctx, "Compound assignment to array elements not yet supported");
            return NULL;
        }

        ASTNode* array_node = left_node->data.array_access.array;
        ASTNode* index_node = left_node->data.array_access.index;

        /* Generate right-hand side value */
        LLVMValue* right_value = generate_expression(ctx, right_node);
        if (!right_value) return NULL;
        right_value = load_value_if_needed(ctx, right_value);
        if (!right_value) return NULL;

        /* Generate array base address */
        LLVMValue* array_value = generate_expression(ctx, array_node);
        if (!array_value) {
            free_llvm_value(right_value);
            return NULL;
        }

        /* Generate index */
        LLVMValue* index_value = generate_expression(ctx, index_node);
        if (!index_value) {
            free_llvm_value(right_value);
            free_llvm_value(array_value);
            return NULL;
        }
        index_value = load_value_if_needed(ctx, index_value);

        /* Get element pointer using getelementptr */
        char* gep_reg = get_next_register(ctx);
        char index_operand[MAX_OPERAND_STRING_LENGTH];
        format_operand(index_value, index_operand, sizeof(index_operand));

        /* Determine element type */
        TypeInfo* element_type = NULL;
        if (array_value->llvm_type && array_value->llvm_type->base_type == TYPE_POINTER) {
            element_type = array_value->llvm_type->return_type;
        } else if (array_value->llvm_type && array_value->llvm_type->base_type == TYPE_ARRAY) {
            element_type = array_value->llvm_type->return_type;
        }
        if (!element_type) {
            element_type = create_type_info(TYPE_INT);
        }

        char* element_type_str = llvm_type_to_string(element_type);
        char* pointer_type_str = llvm_type_to_string(array_value->llvm_type);

        char array_operand[MAX_OPERAND_STRING_LENGTH];
        format_operand(array_value, array_operand, sizeof(array_operand));

        if (array_value->llvm_type && array_value->llvm_type->base_type == TYPE_ARRAY) {
            emit_instruction(ctx, "%%%s = getelementptr %s, %s* %s, i32 0, i32 %s",
                             gep_reg, pointer_type_str, pointer_type_str,
                             array_operand, index_operand);
        } else if (array_value->llvm_type && array_value->llvm_type->base_type == TYPE_POINTER &&
                   array_value->llvm_type->return_type &&
                   array_value->llvm_type->return_type->base_type == TYPE_ARRAY) {
            /* Pointer to array: decay to pointer to first element (GEP 0, index) */
            emit_instruction(ctx, "%%%s = getelementptr %s, %s %s, i32 0, i32 %s",
                             gep_reg, element_type_str, pointer_type_str,
                             array_operand, index_operand);
        } else {
            emit_instruction(ctx, "%%%s = getelementptr %s, %s %s, i32 %s",
                             gep_reg, element_type_str, pointer_type_str,
                             array_operand, index_operand);
        }

        /* Store the value */
        char right_operand[MAX_OPERAND_STRING_LENGTH];
        format_operand(right_value, right_operand, sizeof(right_operand));

        TypeInfo* ptr_to_element = create_pointer_type(duplicate_type_info(element_type));
        char* ptr_type_str = llvm_type_to_string(ptr_to_element);

        emit_instruction(ctx, "store %s %s, %s %%%s",
                         element_type_str, right_operand, ptr_type_str, gep_reg);

        free(element_type_str);
        free(pointer_type_str);
        free(ptr_type_str);
        free_type_info(ptr_to_element);
        free(gep_reg);
        free_llvm_value(array_value);
        free_llvm_value(index_value);

        /* Return the stored value */
        return right_value;
    }

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
        /* If converting to bool, use icmp ne 0 */
        if (symbol->type->base_type == TYPE_BOOL &&
            right_value->llvm_type &&
            right_value->llvm_type->base_type != TYPE_BOOL) {

            char* cmp_reg = get_next_register(ctx);
            emit_instruction(ctx, "%%%s = icmp ne i32 %s, 0", cmp_reg, right_operand);

            /* Update operand to use the bool result */
            snprintf(right_operand, sizeof(right_operand), "%%%s", cmp_reg);

            /* Add to cleanup list? No need, cmp_reg string is managed by us or get_next_register */
            free(cmp_reg);
        }

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
        /* Generate icmp which returns i1, then zext to i32 */
        if (operand->type == LLVM_VALUE_CONSTANT) {
            emit_instruction(ctx, "%%%s = icmp eq i32 %s, 0", result->name,
                             operand_str);
        } else {
            emit_instruction(ctx, "%%%s = icmp eq i32 %%%s, 0", result->name,
                             operand_str);
        }
        /* Convert i1 result to i32 */
        {
            char* zext_reg = get_next_register(ctx);
            emit_instruction(ctx, "%%%s = zext i1 %%%s to i32", zext_reg, result->name);
            /* Free the old name and update to use the zexted value */
            free(result->name);
            result->name = zext_reg;
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

LLVMValue* generate_conditional_op(CodeGenContext* ctx, ASTNode* expr) {
    /* Generate condition */
    LLVMValue* condition = generate_expression(ctx, expr->data.conditional_expr.condition);
    if (!condition)
        return NULL;

    condition = load_value_if_needed(ctx, condition);
    char cond_operand[MAX_OPERAND_STRING_LENGTH];
    format_operand(condition, cond_operand, sizeof(cond_operand));

    /* Create basic blocks */
    char* then_bb = get_next_basic_block(ctx);
    char* else_bb = get_next_basic_block(ctx);
    char* end_bb = get_next_basic_block(ctx);

    /* Evaluate condition and branch */
    /* Evaluate condition and branch */
    char* cmp_reg = NULL;
    if (condition->llvm_type && condition->llvm_type->base_type == TYPE_BOOL) {
        emit_instruction(ctx, "br i1 %s, label %%%s, label %%%s", cond_operand, then_bb, else_bb);
    } else {
        cmp_reg = get_next_register(ctx);
        emit_instruction(ctx, "%%%s = icmp ne i32 %s, 0", cmp_reg, cond_operand);
        emit_instruction(ctx, "br i1 %%%s, label %%%s, label %%%s", cmp_reg, then_bb, else_bb);
    }

    /* Then block - compute true value */
    emit_basic_block_label(ctx, then_bb);
    LLVMValue* then_val = generate_expression(ctx, expr->data.conditional_expr.then_expr);
    then_val = load_value_if_needed(ctx, then_val);
    char then_operand[MAX_OPERAND_STRING_LENGTH];
    format_operand(then_val, then_operand, sizeof(then_operand));
    emit_instruction(ctx, "br label %%%s", end_bb);

    /* Else block - compute false value */
    emit_basic_block_label(ctx, else_bb);
    LLVMValue* else_val = generate_expression(ctx, expr->data.conditional_expr.else_expr);
    else_val = load_value_if_needed(ctx, else_val);
    char else_operand[MAX_OPERAND_STRING_LENGTH];
    format_operand(else_val, else_operand, sizeof(else_operand));
    emit_instruction(ctx, "br label %%%s", end_bb);

    /* End block - phi node */
    emit_basic_block_label(ctx, end_bb);
    char* result_reg = get_next_register(ctx);  /* Get result reg after branches */
    emit_instruction(ctx, "%%%s = phi i32 [ %s, %%%s ], [ %s, %%%s ]",
                     result_reg, then_operand, then_bb, else_operand, else_bb);

    TypeInfo* result_type = create_type_info(TYPE_INT);
    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg, result_type);

    free_llvm_value(condition);
    free_llvm_value(then_val);
    free_llvm_value(else_val);
    free(then_bb);
    free(else_bb);
    free(end_bb);
    if (cmp_reg) free(cmp_reg);
    free(result_reg);

    return result;
}

LLVMValue* generate_cast(CodeGenContext* ctx, ASTNode* expr) {
    TypeInfo* target_type = expr->data.cast_expr.target_type;
    ASTNode* operand_node = expr->data.cast_expr.operand;

    if (!target_type || !operand_node) {
        codegen_error(ctx, "Invalid cast expression");
        return NULL;
    }

    LLVMValue* operand = generate_expression(ctx, operand_node);
    if (!operand)
        return NULL;

    operand = load_value_if_needed(ctx, operand);
    if (!operand)
        return NULL;

    /* Get source and target sizes */
    int src_size = get_type_size(operand->llvm_type);
    int dst_size = get_type_size(target_type);

    /* If same size, return as-is */
    if (src_size == dst_size) {
        TypeInfo* result_type = duplicate_type_info(target_type);
        operand->llvm_type = result_type;
        return operand;
    }

    char* result_reg = get_next_register(ctx);
    char* src_type_str = llvm_type_to_string(operand->llvm_type);
    char* dst_type_str = llvm_type_to_string(target_type);

    char operand_str[MAX_OPERAND_STRING_LENGTH];
    format_operand(operand, operand_str, sizeof(operand_str));

    if (dst_size < src_size) {
        /* Truncate */
        emit_instruction(ctx, "%%%s = trunc %s %s to %s",
                         result_reg, src_type_str, operand_str, dst_type_str);
    } else {
        /* Extend - use sign extension for most types (char, int, etc. are signed) */
        DataType base = operand->llvm_type ? operand->llvm_type->base_type : TYPE_INT;
        bool is_signed = (base == TYPE_INT || base == TYPE_CHAR || base == TYPE_SHORT ||
                          base == TYPE_LONG);
        if (is_signed) {
            emit_instruction(ctx, "%%%s = sext %s %s to %s",
                             result_reg, src_type_str, operand_str, dst_type_str);
        } else {
            emit_instruction(ctx, "%%%s = zext %s %s to %s",
                             result_reg, src_type_str, operand_str, dst_type_str);
        }
    }

    TypeInfo* result_type = duplicate_type_info(target_type);
    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg, result_type);

    free(src_type_str);
    free(dst_type_str);
    free(result_reg);
    free_llvm_value(operand);

    return result;
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
        result->is_lvalue = 1;
        return result;
    }

    /* Global variables */
    if (symbol->is_global) {
        LLVMValue* result =
            create_llvm_value(LLVM_VALUE_GLOBAL, symbol->name,
                              duplicate_type_info(symbol->type));
        result->is_lvalue = 1;
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
    const char* str = string_lit->data.string_literal.string;
    int length = string_lit->data.string_literal.length;

    /* Process escape sequences and remove surrounding quotes */
    char* processed = static_cast<char*>(safe_malloc(length * 2 + 1)); /* Extra space for escapes */
    int j = 0;
    for (int i = 0; i < length; i++) {
        if (str[i] == '\\' && i + 1 < length) {
            /* Handle escape sequences */
            switch (str[i + 1]) {
            case 'n':
                processed[j++] = '\\';
                processed[j++] = '0';
                processed[j++] = 'A';
                i++; /* Skip the 'n' */
                break;
            case 't':
                processed[j++] = '\\';
                processed[j++] = '0';
                processed[j++] = '9';
                i++;
                break;
            case 'r':
                processed[j++] = '\\';
                processed[j++] = '0';
                processed[j++] = 'D';
                i++;
                break;
            case '0':
                processed[j++] = '\\';
                processed[j++] = '0';
                processed[j++] = '0';
                i++;
                break;
            case '\\':
                processed[j++] = '\\';
                processed[j++] = '\\';
                i++;
                break;
            case '"':
                processed[j++] = '\\';
                processed[j++] = '"';
                i++;
                break;
            default:
                processed[j++] = str[i];
                break;
            }
        } else if (str[i] == '\n') {
            processed[j++] = '\\';
            processed[j++] = '0';
            processed[j++] = 'A';
        } else if (str[i] == '\r') {
            processed[j++] = '\\';
            processed[j++] = '0';
            processed[j++] = 'D';
        } else if (str[i] == '\t') {
            processed[j++] = '\\';
            processed[j++] = '0';
            processed[j++] = '9';
        } else if (str[i] != '"') {
            /* Skip surrounding quotes */
            processed[j++] = str[i];
        }
    }
    processed[j] = '\0';

    /* Calculate actual byte length for LLVM [N x i8] */
    int byte_length = 0;
    for (int i = 0; i < length; i++) {
        if (str[i] == '"')
            continue;
        if (str[i] == '\\' && i + 1 < length) {
            byte_length++;
            i++;
        } else if (str[i] == '\n' || str[i] == '\r' || str[i] == '\t') {
            byte_length++;
        } else {
            byte_length++;
        }
    }

    emit_global_declaration(ctx,
                            "@%s = private unnamed_addr constant [%d x i8] "
                            "c\"%s\\00\"",
                            global_name, byte_length + 1, processed);

    free(processed);

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
    case AST_BREAK_STMT:
        if (ctx->loop_break_label) {
            emit_instruction(ctx, "br label %%%s", ctx->loop_break_label);
        }
        break;
    case AST_CONTINUE_STMT:
        if (ctx->loop_continue_label) {
            emit_instruction(ctx, "br label %%%s", ctx->loop_continue_label);
        }
        break;
    case AST_SWITCH_STMT:
        generate_switch_statement(ctx, stmt);
        break;
    default:
        /* Handle other statement types */
        break;
    }
}

void generate_compound_statement(CodeGenContext* ctx, ASTNode* stmt) {
    ASTNode* current = stmt->data.compound_stmt.statements;
    ASTNode* last = NULL;
    while (current) {
        generate_statement(ctx, current);
        last = current;
        current = current->next;
    }

    /* If we're in a loop body and the block didn't end with a terminal instruction,
     * fall through to continue/update block */
    if (ctx->loop_continue_label && last) {
        /* Check if last statement is a terminal statement (break/continue/return) */
        if (last->type != AST_BREAK_STMT &&
            last->type != AST_CONTINUE_STMT &&
            last->type != AST_RETURN_STMT) {
            emit_instruction(ctx, "br label %%%s", ctx->loop_continue_label);
        }
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

    /* If we're in a loop body and this is not a terminal statement,
     * fall through to continue/update block.
     * Note: We only generate this if the last instruction wasn't already a br */
    /* For simplicity, skip this - let loop constructs handle fallthrough */
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
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    auto gc = static_cast<GlobalConstant*>(safe_malloc(sizeof(GlobalConstant)));
    gc->declaration = safe_strdup(buffer);
    gc->next = NULL;

    if (!ctx->global_constants) {
        ctx->global_constants = gc;
    } else {
        GlobalConstant* current = ctx->global_constants;
        while (current->next) {
            current = current->next;
        }
        current->next = gc;
    }
}

static void emit_all_global_constants(CodeGenContext* ctx) {
    auto current = ctx->global_constants;
    if (current) {
        fprintf(ctx->output, "\n; Global constants\n");
    }
    while (current) {
        fprintf(ctx->output, "%s\n", current->declaration);
        current = current->next;
    }
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

void emit_basic_block_label(CodeGenContext* ctx, const char* label) {
    fprintf(ctx->output, "%s:\n", label);
}

/* Runtime support */
void generate_runtime_declarations(CodeGenContext* ctx) {
    emit_comment(ctx, "Runtime function declarations");

    /* Register printf */
    emit_global_declaration(ctx, "declare i32 @printf(i8*, ...)");
    Symbol* printf_sym =
        create_symbol("printf", create_type_info(TYPE_INT));
    printf_sym->is_global = 1;
    add_global_symbol(ctx, printf_sym);

    /* Register scanf */
    emit_global_declaration(ctx, "declare i32 @scanf(i8*, ...)");
    Symbol* scanf_sym =
        create_symbol("scanf", create_type_info(TYPE_INT));
    scanf_sym->is_global = 1;
    add_global_symbol(ctx, scanf_sym);

    /* Register malloc */
    emit_global_declaration(ctx, "declare i8* @malloc(i64)");
    Symbol* malloc_sym = create_symbol(
        "malloc", create_pointer_type(create_type_info(TYPE_CHAR)));
    malloc_sym->is_global = 1;
    add_global_symbol(ctx, malloc_sym);

    /* Register free */
    emit_global_declaration(ctx, "declare void @free(i8*)");
    Symbol* free_sym =
        create_symbol("free", create_type_info(TYPE_VOID));
    free_sym->is_global = 1;
    add_global_symbol(ctx, free_sym);

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

    /* If it's an array, wrap the type */
    ASTNode* dim = decl->data.variable_decl.array_dimensions;
    while (dim) {
        if (dim->type == AST_CONSTANT) {
            int size = dim->data.constant.value.int_val;
            TypeInfo* array_type = create_array_type(symbol_type, size);
            symbol_type = array_type;
        }
        dim = dim->next;
    }

    /* If array size is 0 (from []), update it from string literal initializer */
    if (symbol_type->base_type == TYPE_ARRAY && symbol_type->array_size == 0 &&
        decl->data.variable_decl.initializer &&
        decl->data.variable_decl.initializer->type == AST_STRING_LITERAL) {

        int len = decl->data.variable_decl.initializer->data.string_literal.length;
        symbol_type->array_size = len + 1; // Include null terminator
    }

    Symbol* symbol = create_symbol(decl->data.variable_decl.name, symbol_type);

    if (ctx->current_function_name == NULL) {
        /* Global variable */
        symbol->is_global = 1;
        char* type_str = llvm_type_to_string(symbol->type); /* Use symbol->type which has correct size */
        char init_val_str[1024]; /* Increase buffer size for string */

        /* Check for initializer */
        if (decl->data.variable_decl.initializer &&
            decl->data.variable_decl.initializer->type == AST_CONSTANT) {
            /* Use constant initializer value */
            int init_val = decl->data.variable_decl.initializer->data.constant.value.int_val;
            snprintf(init_val_str, sizeof(init_val_str), "%d", init_val);
        } else if (decl->data.variable_decl.initializer &&
                   decl->data.variable_decl.initializer->type == AST_STRING_LITERAL) {
            /* String literal initializer */
            char escaped[2048];
            escape_string_for_llvm(decl->data.variable_decl.initializer->data.string_literal.string, escaped, sizeof(escaped));

            int str_len = decl->data.variable_decl.initializer->data.string_literal.length;
            int array_size = symbol->type->array_size;

            char buffer[4096];
            snprintf(buffer, sizeof(buffer), "c\"%s", escaped);

            /* Add padding \00 if needed */
            for (int i = str_len; i < array_size; i++) {
                if (strlen(buffer) + 4 < sizeof(buffer)) {
                    strcat(buffer, "\\00");
                }
            }
            strcat(buffer, "\"");

            strncpy(init_val_str, buffer, sizeof(init_val_str));
            init_val_str[sizeof(init_val_str)-1] = '\0';
        } else {
            /* Use default value */
            char* default_val = get_default_value(decl->data.variable_decl.type);
            snprintf(init_val_str, sizeof(init_val_str), "%s", default_val);
            free(default_val);
        }

        emit_global_declaration(ctx, "@%s = global %s %s", symbol->name,
                                type_str, init_val_str);

        add_global_symbol(ctx, symbol);
        free(type_str);
    } else {
        /* Local variable */
        int array_size = 0;
        if (symbol->type->base_type == TYPE_ARRAY) {
            array_size = symbol->type->array_size;
        }

        if (array_size > 0) {
            /* Array declaration: allocate [N x type] and store as pointer to element */
            char* type_str = llvm_type_to_string(symbol->type);
            emit_instruction(ctx, "%%%s = alloca %s", symbol->name, type_str);
            free(type_str);

            /* Handle array initialization */
            if (decl->data.variable_decl.initializer) {
                char* element_type_str = llvm_type_to_string(symbol->type->return_type);

                if (decl->data.variable_decl.initializer->type == AST_INITIALIZER_LIST) {
                    ASTNode* item = decl->data.variable_decl.initializer->data.initializer_list.items;
                    int index = 0;
                    while (item && index < array_size) {
                        LLVMValue* val = generate_expression(ctx, item);
                        if (val) {
                             val = load_value_if_needed(ctx, val);
                             /* Verify type match? For now verify strictness or implicit cast logic */

                             char val_operand[MAX_OPERAND_STRING_LENGTH];
                             format_operand(val, val_operand, sizeof(val_operand));

                             char* gep_reg = get_next_register(ctx);
                             /* symbol->name is pointer to array [N x T]* */
                             emit_instruction(ctx, "%%%s = getelementptr [%d x %s], [%d x %s]* %%%s, i32 0, i32 %d",
                                              gep_reg, array_size, element_type_str,
                                              array_size, element_type_str, symbol->name, index);

                             emit_instruction(ctx, "store %s %s, %s* %%%s",
                                              element_type_str, val_operand, element_type_str, gep_reg);

                             free(gep_reg);
                             free_llvm_value(val);
                        }
                        item = item->next;
                        index++;
                    }
                } else if (decl->data.variable_decl.initializer->type == AST_STRING_LITERAL) {
                    /* String literal initialization: char s[] = "abc"; */
                    const char* s = decl->data.variable_decl.initializer->data.string_literal.string;
                    int len = decl->data.variable_decl.initializer->data.string_literal.length;

                    for (int i = 0; i < array_size; i++) {
                        char val_operand[16];
                        /* Fill with string chars, then 0 */
                        unsigned char c = (i < len) ? (unsigned char)s[i] : (i == len ? 0 : 0);
                        snprintf(val_operand, sizeof(val_operand), "%d", c);

                        char* gep_reg = get_next_register(ctx);
                        emit_instruction(ctx, "%%%s = getelementptr [%d x %s], [%d x %s]* %%%s, i32 0, i32 %d",
                                         gep_reg, array_size, element_type_str,
                                         array_size, element_type_str, symbol->name, i);

                        emit_instruction(ctx, "store %s %s, %s* %%%s",
                                         element_type_str, val_operand, element_type_str, gep_reg);
                        free(gep_reg);
                    }
                }
                free(element_type_str);
            }
            /* Symbol type remains TYPE_ARRAY, so load_value_if_needed will handle decay properly */


        } else {
            /* Regular variable */
            char* type_str = llvm_type_to_string(decl->data.variable_decl.type);
            emit_instruction(ctx, "%%%s = alloca %s", symbol->name, type_str);
            free(type_str);

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
        }

        add_local_symbol(ctx, symbol);
    }
}

/* Utility functions */
LLVMValue* create_llvm_value(LLVMValueType type, const char* name,
                             TypeInfo* llvm_type) {
    auto value = static_cast<LLVMValue*>(safe_malloc(sizeof(LLVMValue)));
    value->type = type;
    value->name = safe_strdup(name);
    value->llvm_type = llvm_type;
    value->is_lvalue = 0;
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
    case TYPE_BOOL:
        return safe_strdup("i1");
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
    case TYPE_BOOL:
        return 1;
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

    condition = load_value_if_needed(ctx, condition);

    /* Create basic blocks */
    char* then_label = get_next_basic_block(ctx);
    char* else_label = get_next_basic_block(ctx);
    char* end_label = get_next_basic_block(ctx);

    /* Convert condition to i1 for branch */
    char cond_operand[MAX_OPERAND_STRING_LENGTH];
    format_operand(condition, cond_operand, sizeof(cond_operand));

    /* Branch based on condition */
    if (condition->llvm_type && condition->llvm_type->base_type == TYPE_BOOL) {
        emit_instruction(ctx, "br i1 %s, label %%%s, label %%%s", cond_operand, then_label, else_label);
    } else {
        char* cmp_reg = get_next_register(ctx);
        emit_instruction(ctx, "%%%s = icmp ne i32 %s, 0", cmp_reg, cond_operand);
        emit_instruction(ctx, "br i1 %%%s, label %%%s, label %%%s", cmp_reg, then_label, else_label);
        free(cmp_reg);
    }

    /* Then block */
    emit_basic_block_label(ctx, then_label);
    generate_statement(ctx, stmt->data.if_stmt.then_stmt);
    /* Generate fallthrough br if then_stmt is not a compound statement */
    /* Compound statements in loops handle their own fallthrough */
    if (stmt->data.if_stmt.then_stmt->type != AST_COMPOUND_STMT) {
        emit_instruction(ctx, "br label %%%s", end_label);
    } else if (!ctx->loop_continue_label) {
        /* Compound statement not in a loop - need fallthrough */
        emit_instruction(ctx, "br label %%%s", end_label);
    }

    /* Else block */
    if (stmt->data.if_stmt.else_stmt) {
        emit_basic_block_label(ctx, else_label);
        generate_statement(ctx, stmt->data.if_stmt.else_stmt);
        /* Generate fallthrough br if else_stmt is not a compound statement */
        if (stmt->data.if_stmt.else_stmt->type != AST_COMPOUND_STMT) {
            emit_instruction(ctx, "br label %%%s", end_label);
        } else if (!ctx->loop_continue_label) {
            /* Compound statement not in a loop - need fallthrough */
            emit_instruction(ctx, "br label %%%s", end_label);
        }
    } else {
        /* No else clause - else_label just falls through to end_label */
        emit_basic_block_label(ctx, else_label);
        emit_instruction(ctx, "br label %%%s", end_label);
    }

    /* End block */
    emit_basic_block_label(ctx, end_label);

    free_llvm_value(condition);
    free(then_label);
    free(else_label);
    free(end_label);
}

void generate_while_statement(CodeGenContext* ctx, ASTNode* stmt) {
    emit_instruction(ctx, "; while statement");

    char* cond_bb = get_next_basic_block(ctx);
    char* body_bb = get_next_basic_block(ctx);
    char* end_bb = get_next_basic_block(ctx);

    /* Save previous loop labels */
    char* saved_break = ctx->loop_break_label;
    char* saved_continue = ctx->loop_continue_label;

    /* Set current loop labels */
    ctx->loop_break_label = end_bb;
    ctx->loop_continue_label = cond_bb;

    /* Jump to condition */
    emit_instruction(ctx, "br label %%%s", cond_bb);

    /* Condition block */
    emit_basic_block_label(ctx, cond_bb);
    LLVMValue* condition = generate_expression(ctx, stmt->data.while_stmt.condition);
    if (condition) {
        condition = load_value_if_needed(ctx, condition);
        char cond_operand[MAX_OPERAND_STRING_LENGTH];
        format_operand(condition, cond_operand, sizeof(cond_operand));

        if (condition->llvm_type && condition->llvm_type->base_type == TYPE_BOOL) {
            emit_instruction(ctx, "br i1 %s, label %%%s, label %%%s", cond_operand, body_bb, end_bb);
        } else {
            char* cmp_reg = get_next_register(ctx);
            emit_instruction(ctx, "%%%s = icmp ne i32 %s, 0", cmp_reg, cond_operand);
            emit_instruction(ctx, "br i1 %%%s, label %%%s, label %%%s", cmp_reg, body_bb, end_bb);
            free(cmp_reg);
        }
        free_llvm_value(condition);
    }

    /* Body block */
    emit_basic_block_label(ctx, body_bb);
    generate_statement(ctx, stmt->data.while_stmt.body);
    emit_instruction(ctx, "br label %%%s", cond_bb);

    /* End block */
    emit_basic_block_label(ctx, end_bb);

    /* Restore loop labels */
    ctx->loop_break_label = saved_break;
    ctx->loop_continue_label = saved_continue;

    free(cond_bb);
    free(body_bb);
    free(end_bb);
}

void generate_for_statement(CodeGenContext* ctx, ASTNode* stmt) {
    emit_instruction(ctx, "; for statement");

    char* cond_bb = get_next_basic_block(ctx);
    char* body_bb = get_next_basic_block(ctx);
    char* update_bb = get_next_basic_block(ctx);
    char* end_bb = get_next_basic_block(ctx);

    /* Save previous loop labels */
    char* saved_break = ctx->loop_break_label;
    char* saved_continue = ctx->loop_continue_label;

    /* Set current loop labels */
    ctx->loop_break_label = end_bb;
    ctx->loop_continue_label = update_bb;

    /* Init */
    if (stmt->data.for_stmt.init) {
        /* C99: init can be a declaration or an expression */
        if (stmt->data.for_stmt.init->type == AST_VARIABLE_DECL) {
            generate_declaration(ctx, stmt->data.for_stmt.init);
        } else {
            generate_expression(ctx, stmt->data.for_stmt.init);
        }
    }

    /* Jump to condition */
    emit_instruction(ctx, "br label %%%s", cond_bb);

    /* Condition block */
    emit_basic_block_label(ctx, cond_bb);
    if (stmt->data.for_stmt.condition) {
        LLVMValue* condition = generate_expression(ctx, stmt->data.for_stmt.condition);
        if (condition) {
            condition = load_value_if_needed(ctx, condition);
            char cond_operand[MAX_OPERAND_STRING_LENGTH];
            format_operand(condition, cond_operand, sizeof(cond_operand));

            if (condition->llvm_type && condition->llvm_type->base_type == TYPE_BOOL) {
                emit_instruction(ctx, "br i1 %s, label %%%s, label %%%s", cond_operand, body_bb, end_bb);
            } else {
                char* cmp_reg = get_next_register(ctx);
                emit_instruction(ctx, "%%%s = icmp ne i32 %s, 0", cmp_reg, cond_operand);
                emit_instruction(ctx, "br i1 %%%s, label %%%s, label %%%s", cmp_reg, body_bb, end_bb);
                free(cmp_reg);
            }
            free_llvm_value(condition);
        }
    } else {
        /* No condition = always true */
        emit_instruction(ctx, "br label %%%s", body_bb);
    }

    /* Body block - must be a compound statement for proper control flow */
    emit_basic_block_label(ctx, body_bb);
    generate_statement(ctx, stmt->data.for_stmt.body);
    /* Don't generate fallthrough br - compound statement handles it */

    /* Update block (implicit fallthrough target for normal statements) */
    emit_basic_block_label(ctx, update_bb);
    if (stmt->data.for_stmt.update) {
        LLVMValue* update_result = generate_expression(ctx, stmt->data.for_stmt.update);
        if (update_result) {
            free_llvm_value(update_result);
        }
    }
    emit_instruction(ctx, "br label %%%s", cond_bb);

    /* End block */
    emit_basic_block_label(ctx, end_bb);

    /* Restore loop labels */
    ctx->loop_break_label = saved_break;
    ctx->loop_continue_label = saved_continue;

    free(cond_bb);
    free(body_bb);
    free(update_bb);
    free(end_bb);
}

void generate_switch_statement(CodeGenContext* ctx, ASTNode* stmt) {
    if (!stmt || !stmt->data.switch_stmt.expression) {
        codegen_error(ctx, "Invalid switch statement");
        return;
    }

    emit_comment(ctx, "switch statement");

    /* Generate switch expression */
    LLVMValue* switch_val = generate_expression(ctx, stmt->data.switch_stmt.expression);
    if (!switch_val) return;
    switch_val = load_value_if_needed(ctx, switch_val);

    char switch_operand[MAX_OPERAND_STRING_LENGTH];
    format_operand(switch_val, switch_operand, sizeof(switch_operand));

    /* Create end label for break statements */
    char* end_bb = get_next_basic_block(ctx);
    char* saved_break = ctx->loop_break_label;
    ctx->loop_break_label = end_bb;

    /* Collect case statements from the body */
    ASTNode* body = stmt->data.switch_stmt.body;
    if (body && body->type == AST_COMPOUND_STMT) {
        ASTNode* current = body->data.compound_stmt.statements;
        ASTNode* default_stmt = NULL;
        char* default_bb = NULL;

        /* First pass: generate labels for each case */
        while (current) {
            if (current->type == AST_CASE_STMT) {
                char* case_bb = get_next_basic_block(ctx);
                int case_val = current->data.case_stmt.value->data.constant.value.int_val;

                /* Compare and branch */
                char* cmp_reg = get_next_register(ctx);
                emit_instruction(ctx, "%%%s = icmp eq i32 %s, %d", cmp_reg, switch_operand, case_val);

                char* next_check_bb = get_next_basic_block(ctx);
                emit_instruction(ctx, "br i1 %%%s, label %%%s, label %%%s", cmp_reg, case_bb, next_check_bb);

                /* Case body */
                emit_basic_block_label(ctx, case_bb);
                generate_statement(ctx, current->data.case_stmt.statement);
                emit_instruction(ctx, "br label %%%s", end_bb);

                /* Continue checking */
                emit_basic_block_label(ctx, next_check_bb);

                free(cmp_reg);
                free(case_bb);
                free(next_check_bb);
            } else if (current->type == AST_DEFAULT_STMT) {
                default_stmt = current;
                default_bb = get_next_basic_block(ctx);
            }
            current = current->next;
        }

        /* Handle default case or fall through to end */
        if (default_stmt && default_bb) {
            emit_instruction(ctx, "br label %%%s", default_bb);
            emit_basic_block_label(ctx, default_bb);
            generate_statement(ctx, default_stmt->data.case_stmt.statement);
            emit_instruction(ctx, "br label %%%s", end_bb);
            free(default_bb);
        } else {
            emit_instruction(ctx, "br label %%%s", end_bb);
        }
    }

    /* End block */
    emit_basic_block_label(ctx, end_bb);

    /* Restore break label */
    ctx->loop_break_label = saved_break;

    free(end_bb);
    free_llvm_value(switch_val);
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

        arg_val = load_value_if_needed(ctx, arg_val);

        /* Promote i1 (bool) to i32 (int) for varargs compatibility */
        if (arg_val->llvm_type && arg_val->llvm_type->base_type == TYPE_BOOL) {
            char cond_operand[MAX_OPERAND_STRING_LENGTH];
            format_operand(arg_val, cond_operand, sizeof(cond_operand));

            char* zext_reg = get_next_register(ctx);
            emit_instruction(ctx, "%%%s = zext i1 %s to i32", zext_reg, cond_operand);

            /* Create new LLVMValue for the promoted argument */
            TypeInfo* i32_type = create_type_info(TYPE_INT);
            // We need to manage memory for i32_type correctly, but simplify for now
            // Or better, reuse existing type info creation if possible, but create_type_info mallocs new.
            // It will leak if not freed, but let's accept it for now or free arg_val->llvm_type?

            LLVMValue* promoted_val = create_llvm_value(LLVM_VALUE_REGISTER, zext_reg, i32_type);

            free_llvm_value(arg_val);
            arg_val = promoted_val;
            free(zext_reg);
        }

        if (arg_count > 0) {
            strcat(arg_list, ", ");
        }

        char arg_spec[256];
        char* type_str = llvm_type_to_string(arg_val->llvm_type);

        if (arg_val->type == LLVM_VALUE_CONSTANT) {
            /* For constants, use the value directly */
            snprintf(arg_spec, sizeof(arg_spec), "%s %s", type_str, arg_val->name);
        } else if (arg_val->type == LLVM_VALUE_GLOBAL) {
            /* For global values (like string literals), use @ notation */
            snprintf(arg_spec, sizeof(arg_spec), "%s @%s", type_str, arg_val->name);
        } else {
            /* For variables/registers, use register notation */
            snprintf(arg_spec, sizeof(arg_spec), "%s %%%s", type_str, arg_val->name);
        }
        strcat(arg_list, arg_spec);
        free(type_str);

        free_llvm_value(arg_val);
        arg = arg->next;
        arg_count++;
    }

    char* result_reg = get_next_register(ctx);
    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                                          create_type_info(TYPE_INT));

    /* Build function prototype for the call instruction (needed for variadic calls) */
    char proto[1024] = "";
    if (strcmp(func_name, "printf") == 0 || strcmp(func_name, "scanf") == 0) {
        snprintf(proto, sizeof(proto), "(i8*, ...) ");
    }

    emit_instruction(ctx, "%%%s = call i32 %s@%s(%s)", result->name, proto, func_name,
                     arg_list);

    /* Free the original result_reg since create_llvm_value duplicated it */
    free(result_reg);
    return result;
}

LLVMValue* generate_array_access(CodeGenContext* ctx, ASTNode* access) {
    if (!access || access->type != AST_ARRAY_ACCESS) {
        codegen_error(ctx, "Invalid array access node");
        return NULL;
    }

    ASTNode* array_node = access->data.array_access.array;
    ASTNode* index_node = access->data.array_access.index;

    if (!array_node || !index_node) {
        codegen_error(ctx, "Invalid array access: missing array or index");
        return NULL;
    }

    /* Generate array (should evaluate to pointer) */
    LLVMValue* array_value = generate_expression(ctx, array_node);
    if (!array_value) {
        codegen_error(ctx, "Failed to generate array expression");
        return NULL;
    }

    /* Generate index */
    LLVMValue* index_value = generate_expression(ctx, index_node);
    if (!index_value) {
        codegen_error(ctx, "Failed to generate index expression");
        free_llvm_value(array_value);
        return NULL;
    }

    index_value = load_value_if_needed(ctx, index_value);

    /* Get element pointer */
    char* gep_reg = get_next_register(ctx);
    char array_operand[MAX_OPERAND_STRING_LENGTH];
    char index_operand[MAX_OPERAND_STRING_LENGTH];
    format_operand(array_value, array_operand, sizeof(array_operand));
    format_operand(index_value, index_operand, sizeof(index_operand));

    /* Determine the element type from the array's pointer type */
    TypeInfo* element_type = NULL;
    if (array_value->llvm_type && array_value->llvm_type->base_type == TYPE_POINTER) {
        element_type = array_value->llvm_type->return_type;
    } else if (array_value->llvm_type && array_value->llvm_type->base_type == TYPE_ARRAY) {
        element_type = array_value->llvm_type->return_type;
    }

    if (!element_type) {
        /* Default to int */
        element_type = create_type_info(TYPE_INT);
    }

    char* element_type_str = llvm_type_to_string(element_type);
    char* pointer_type_str = llvm_type_to_string(array_value->llvm_type);

    if (array_value->llvm_type && array_value->llvm_type->base_type == TYPE_ARRAY) {
        emit_instruction(ctx, "%%%s = getelementptr %s, %s* %s, i32 0, i32 %s",
                         gep_reg, pointer_type_str, pointer_type_str,
                         array_operand, index_operand);
    } else if (array_value->llvm_type && array_value->llvm_type->base_type == TYPE_POINTER &&
               array_value->llvm_type->return_type &&
               array_value->llvm_type->return_type->base_type == TYPE_ARRAY) {
        /* Pointer to array: decay to pointer to first element (GEP 0, index) */
        emit_instruction(ctx, "%%%s = getelementptr %s, %s %s, i32 0, i32 %s",
                         gep_reg, element_type_str, pointer_type_str,
                         array_operand, index_operand);
    } else {
        emit_instruction(ctx, "%%%s = getelementptr %s, %s %s, i32 %s",
                         gep_reg, element_type_str, pointer_type_str,
                         array_operand, index_operand);
    }

    free(element_type_str);
    free(pointer_type_str);

    /* Return the GEP result as lvalue (address) */
    /* If the caller needs the value, it will call load_value_if_needed */
    TypeInfo* result_type = duplicate_type_info(element_type);
    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, gep_reg, result_type);
    result->is_lvalue = 1; /* Result is an address */

    free(gep_reg);

    free_llvm_value(array_value);
    free_llvm_value(index_value);

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
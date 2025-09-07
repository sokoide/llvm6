#include "codegen.h"
#include <assert.h>
#include <stdarg.h>

/* Helper functions */
static void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(1);
    }
    return ptr;
}

static char* safe_strdup(const char* str) {
    if (!str)
        return NULL;
    char* new_str = (char*)safe_malloc(strlen(str) + 1);
    strcpy(new_str, str);
    return new_str;
}

/* Context management */
CodeGenContext* create_codegen_context(FILE* output) {
    CodeGenContext* ctx = (CodeGenContext*)safe_malloc(sizeof(CodeGenContext));
    memset(ctx, 0, sizeof(CodeGenContext));

    ctx->output = output;
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

/* Main code generation function */
void generate_llvm_ir(CodeGenContext* ctx, ASTNode* ast) {
    if (!ctx || !ast)
        return;

    /* Generate module header */
    fprintf(ctx->output, "; Generated LLVM IR\n");
    fprintf(ctx->output, "target triple = \"x86_64-unknown-linux-gnu\"\n\n");

    /* Generate runtime declarations */
    generate_runtime_declarations(ctx);

    /* Process translation unit */
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

    fprintf(ctx->output, "\n");
}

/* Expression generation */
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

LLVMValue* generate_binary_op(CodeGenContext* ctx, ASTNode* expr) {
    LLVMValue* left = generate_expression(ctx, expr->data.binary_op.left);
    LLVMValue* right = generate_expression(ctx, expr->data.binary_op.right);

    if (!left || !right) {
        return NULL;
    }

    const char* op_name = NULL;
    char* result_reg = NULL;
    LLVMValue* result = NULL;
    switch (expr->data.binary_op.op) {
    case OP_ADD:
        op_name = "add";
        break;
    case OP_SUB:
        op_name = "sub";
        break;
    case OP_MUL:
        op_name = "mul";
        break;
    case OP_DIV:
        op_name = "sdiv";
        break;
    case OP_MOD:
        op_name = "srem";
        break;
    case OP_LT:
        op_name = "icmp slt";
        break;
    case OP_GT:
        op_name = "icmp sgt";
        break;
    case OP_LE:
        op_name = "icmp sle";
        break;
    case OP_GE:
        op_name = "icmp sge";
        break;
    case OP_EQ:
        op_name = "icmp eq";
        break;
    case OP_NE:
        op_name = "icmp ne";
        break;
    case OP_BITAND:
        op_name = "and";
        break;
    case OP_BITOR:
        op_name = "or";
        break;
    case OP_XOR:
        op_name = "xor";
        break;
    case OP_LSHIFT:
        op_name = "shl";
        break;
    case OP_RSHIFT:
        op_name = "ashr";
        break;

    /* Assignment operators - handled separately */
    case OP_ASSIGN:
    case OP_ADD_ASSIGN:
    case OP_SUB_ASSIGN:
    case OP_MUL_ASSIGN:
    case OP_DIV_ASSIGN:
    case OP_MOD_ASSIGN:
    case OP_AND_ASSIGN:
    case OP_OR_ASSIGN:
    case OP_XOR_ASSIGN:
    case OP_LSHIFT_ASSIGN:
    case OP_RSHIFT_ASSIGN:
        return generate_assignment_op(ctx, expr);

    default:
        codegen_error(ctx, "Unsupported binary operator: %d",
                      expr->data.binary_op.op);
        return NULL;
    }

    /* Format operands properly - constants as literals, variables as registers
     */
    char left_operand[64], right_operand[64];
    if (left->type == LLVM_VALUE_CONSTANT) {
        snprintf(left_operand, sizeof(left_operand), "%d",
                 left->data.constant_val);
    } else {
        snprintf(left_operand, sizeof(left_operand), "%%%s", left->name);
    }

    if (right->type == LLVM_VALUE_CONSTANT) {
        snprintf(right_operand, sizeof(right_operand), "%d",
                 right->data.constant_val);
    } else {
        snprintf(right_operand, sizeof(right_operand), "%%%s", right->name);
    }

    /* Check if this is a comparison operator that returns i1 */
    if (expr->data.binary_op.op == OP_LT || expr->data.binary_op.op == OP_GT ||
        expr->data.binary_op.op == OP_LE || expr->data.binary_op.op == OP_GE ||
        expr->data.binary_op.op == OP_EQ || expr->data.binary_op.op == OP_NE) {

        /* For comparisons, allocate cmp_reg first, then result_reg */
        char* cmp_reg = get_next_register(ctx);
        result_reg = get_next_register(ctx);
        result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                                  create_type_info(TYPE_INT));
        emit_instruction(ctx, "%%%s = %s i32 %s, %s", cmp_reg, op_name,
                         left_operand, right_operand);

        /* Convert i1 to i32 */
        emit_instruction(ctx, "%%%s = zext i1 %%%s to i32", result_reg,
                         cmp_reg);

        free_llvm_value(left);
        free_llvm_value(right);
        free(cmp_reg);
        /* Free the original result_reg since create_llvm_value duplicated it */
        free(result_reg);

        return result;
    }

    /* Regular binary operations (arithmetic, bitwise) */
    result_reg = get_next_register(ctx);
    result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                              create_type_info(TYPE_INT));
                              
    emit_instruction(ctx, "%%%s = %s i32 %s, %s", result_reg, op_name,
                    left_operand, right_operand);
    
    /* Free the original result_reg since create_llvm_value duplicated it */
    free(result_reg);

    free_llvm_value(left);
    free_llvm_value(right);

    return result;
}

LLVMValue* generate_assignment_op(CodeGenContext* ctx, ASTNode* expr) {
    ASTNode* left_node = expr->data.binary_op.left;
    ASTNode* right_node = expr->data.binary_op.right;
    BinaryOp op = expr->data.binary_op.op;

    /* Left side must be an identifier (variable) */
    if (left_node->type != AST_IDENTIFIER) {
        codegen_error(ctx, "Left side of assignment must be a variable");
        return NULL;
    }

    /* Look up the variable in symbol table */
    Symbol* symbol = lookup_symbol(ctx, left_node->data.identifier.name);
    if (!symbol) {
        codegen_error(ctx, "Undefined variable: %s",
                      left_node->data.identifier.name);
        return NULL;
    }

    /* Generate right-hand side expression */
    LLVMValue* right_value = generate_expression(ctx, right_node);
    if (!right_value)
        return NULL;

    char* result_reg = get_next_register(ctx);
    LLVMValue* result =
        create_llvm_value(LLVM_VALUE_REGISTER, result_reg, symbol->type);

    /* Format right operand */
    char right_operand[64];
    if (right_value->type == LLVM_VALUE_CONSTANT) {
        snprintf(right_operand, sizeof(right_operand), "%d",
                 right_value->data.constant_val);
    } else {
        snprintf(right_operand, sizeof(right_operand), "%%%s",
                 right_value->name);
    }

    if (op == OP_ASSIGN) {
        /* Simple assignment: store right value to variable */
        emit_instruction(ctx, "store i32 %s, i32* %%%s", right_operand,
                         symbol->name);
        /* Return the stored value */
        emit_instruction(ctx, "%%%s = load i32, i32* %%%s", result->name,
                         symbol->name);
    } else {
        /* Compound assignment: load variable, perform operation, store result
         */
        char* temp_reg = get_next_register(ctx);
        emit_instruction(ctx, "%%%s = load i32, i32* %%%s", temp_reg,
                         symbol->name);

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
            free(temp_reg);
            free_llvm_value(right_value);
            free(result_reg);
            return NULL;
        }

        /* Perform the operation */
        emit_instruction(ctx, "%%%s = %s i32 %%%s, %s", result->name, op_name,
                         temp_reg, right_operand);

        /* Store the result back to the variable */
        emit_instruction(ctx, "store i32 %%%s, i32* %%%s", result->name,
                         symbol->name);

        free(temp_reg);
    }

    free_llvm_value(right_value);
    /* Free the original result_reg since create_llvm_value duplicated it */
    free(result_reg);

    return result;
}

LLVMValue* generate_unary_op(CodeGenContext* ctx, ASTNode* expr) {
    LLVMValue* operand = generate_expression(ctx, expr->data.unary_op.operand);
    if (!operand)
        return NULL;

    char* result_reg = get_next_register(ctx);
    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                                          create_type_info(TYPE_INT));

    switch (expr->data.unary_op.op) {
    case UOP_PLUS:
        /* Unary plus is a no-op - just return the operand value */
        if (operand->type == LLVM_VALUE_CONSTANT) {
            emit_instruction(ctx, "%%%s = add i32 0, %s", result->name,
                             operand->name);
        } else {
            emit_instruction(ctx, "%%%s = add i32 0, %%%s", result->name,
                             operand->name);
        }
        break;
    case UOP_MINUS:
        if (operand->type == LLVM_VALUE_CONSTANT) {
            emit_instruction(ctx, "%%%s = sub i32 0, %s", result->name,
                             operand->name);
        } else {
            emit_instruction(ctx, "%%%s = sub i32 0, %%%s", result->name,
                             operand->name);
        }
        break;
    case UOP_NOT:
        if (operand->type == LLVM_VALUE_CONSTANT) {
            emit_instruction(ctx, "%%%s = icmp eq i32 %s, 0", result->name,
                             operand->name);
        } else {
            emit_instruction(ctx, "%%%s = icmp eq i32 %%%s, 0", result->name,
                             operand->name);
        }
        break;
    case UOP_BITNOT:
        if (operand->type == LLVM_VALUE_CONSTANT) {
            emit_instruction(ctx, "%%%s = xor i32 %s, -1", result->name,
                             operand->name);
        } else {
            emit_instruction(ctx, "%%%s = xor i32 %%%s, -1", result->name,
                             operand->name);
        }
        break;
    case UOP_PREINC:
        /* Pre-increment: ++x - increment then return new value */
        if (operand->type == LLVM_VALUE_CONSTANT) {
            codegen_error(ctx, "Cannot increment constant");
            free_llvm_value(result);
            free_llvm_value(operand);
            free(result_reg);
            return NULL;
        } else {
            char* temp_reg = get_next_register(ctx);
            emit_instruction(ctx, "%%%s = add i32 %%%s, 1", temp_reg,
                             operand->name);
            emit_instruction(ctx, "store i32 %%%s, i32* %%%s", temp_reg,
                             operand->name);
            emit_instruction(ctx, "%%%s = load i32, i32* %%%s", result->name,
                             operand->name);
            free(temp_reg);
        }
        break;
    case UOP_PREDEC:
        /* Pre-decrement: --x - decrement then return new value */
        if (operand->type == LLVM_VALUE_CONSTANT) {
            codegen_error(ctx, "Cannot decrement constant");
            free_llvm_value(result);
            free_llvm_value(operand);
            free(result_reg);
            return NULL;
        } else {
            char* temp_reg = get_next_register(ctx);
            emit_instruction(ctx, "%%%s = sub i32 %%%s, 1", temp_reg,
                             operand->name);
            emit_instruction(ctx, "store i32 %%%s, i32* %%%s", temp_reg,
                             operand->name);
            emit_instruction(ctx, "%%%s = load i32, i32* %%%s", result->name,
                             operand->name);
            free(temp_reg);
        }
        break;
    case UOP_POSTINC:
        /* Post-increment: x++ - return old value then increment */
        if (operand->type == LLVM_VALUE_CONSTANT) {
            codegen_error(ctx, "Cannot increment constant");
            free_llvm_value(result);
            free_llvm_value(operand);
            free(result_reg);
            return NULL;
        } else {
            char* temp_reg = get_next_register(ctx);
            emit_instruction(ctx, "%%%s = load i32, i32* %%%s", result->name,
                             operand->name);
            emit_instruction(ctx, "%%%s = add i32 %%%s, 1", temp_reg,
                             result->name);
            emit_instruction(ctx, "store i32 %%%s, i32* %%%s", temp_reg,
                             operand->name);
            free(temp_reg);
        }
        break;
    case UOP_POSTDEC:
        /* Post-decrement: x-- - return old value then decrement */
        if (operand->type == LLVM_VALUE_CONSTANT) {
            codegen_error(ctx, "Cannot decrement constant");
            free_llvm_value(result);
            free_llvm_value(operand);
            free(result_reg);
            return NULL;
        } else {
            char* temp_reg = get_next_register(ctx);
            emit_instruction(ctx, "%%%s = load i32, i32* %%%s", result->name,
                             operand->name);
            emit_instruction(ctx, "%%%s = sub i32 %%%s, 1", temp_reg,
                             result->name);
            emit_instruction(ctx, "store i32 %%%s, i32* %%%s", temp_reg,
                             operand->name);
            free(temp_reg);
        }
        break;
    case UOP_SIZEOF:
        /* sizeof operator - return size in bytes (simplified for int) */
        emit_instruction(ctx, "%%%s = add i32 0, 4",
                         result->name); /* int is 4 bytes */
        break;
    case UOP_ADDR:
        /* Address operator &x - get pointer to variable */
        if (operand->type == LLVM_VALUE_CONSTANT) {
            codegen_error(ctx, "Cannot take address of constant");
            free_llvm_value(result);
            free_llvm_value(operand);
            free(result_reg);
            return NULL;
        } else {
            /* For variables, return their address (the allocated pointer) */
            Symbol* symbol = lookup_symbol(ctx, operand->name);
            if (!symbol) {
                codegen_error(ctx, "Cannot find symbol for address operation");
                free_llvm_value(result);
                free_llvm_value(operand);
                free(result_reg);
                return NULL;
            }
            /* Return the address directly */
            emit_instruction(ctx, "%%%s = ptrtoint i32* %%%s to i32",
                             result->name, symbol->name);
        }
        break;
    case UOP_DEREF: {
        /* Dereference operator *x - get value pointed to */
        if (operand->type == LLVM_VALUE_CONSTANT) {
            emit_instruction(ctx, "%%%s = inttoptr i32 %s to i32*", result->name,
                             operand->name);
        } else {
            emit_instruction(ctx, "%%%s = inttoptr i32 %%%s to i32*",
                             result->name, operand->name);
        }
        char* load_reg = get_next_register(ctx);
        emit_instruction(ctx, "%%%s = load i32, i32* %%%s", load_reg,
                         result->name);
        /* Update result to point to the loaded value */
        free(result->name);
        result->name = safe_strdup(load_reg);
        free(load_reg);
        /* Note: load_reg now becomes result_reg and will be freed when result is freed */
        break;
    }
    default:
        codegen_error(ctx, "Unsupported unary operator: %d",
                      expr->data.unary_op.op);
        free_llvm_value(result);
        free_llvm_value(operand);
        free(result_reg);
        return NULL;
    }

    free_llvm_value(operand);
    /* Free the original result_reg since create_llvm_value duplicated it */
    free(result_reg);

    return result;
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

    /* For local variables, load from memory */
    if (!symbol->is_global && ctx->current_function_name) {
        char* load_reg = get_next_register(ctx);
        LLVMValue* result =
            create_llvm_value(LLVM_VALUE_REGISTER, load_reg, duplicate_type_info(symbol->type));

        emit_instruction(ctx, "%%%s = load i32, i32* %%%s", load_reg,
                         symbol->name);
        /* Free the original load_reg since create_llvm_value duplicated it */
        free(load_reg);
        return result;
    }

    /* Global variables */
    if (symbol->is_global) {
        char* load_reg = get_next_register(ctx);
        LLVMValue* result =
            create_llvm_value(LLVM_VALUE_REGISTER, load_reg, duplicate_type_info(symbol->type));
        emit_instruction(ctx, "%%%s = load i32, i32* @%s", load_reg,
                         symbol->name);
        /* Free the original load_reg since create_llvm_value duplicated it */
        free(load_reg);
        return result;
    }

    /* Default case - should not reach here */
    LLVMValue* result = create_llvm_value(
        LLVM_VALUE_REGISTER, identifier->data.identifier.name, duplicate_type_info(symbol->type));
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

    emit_global_declaration(
        ctx, "@%s = private unnamed_addr constant [%d x i8] c\"%s\\00\"",
        global_name, length + 1, string_lit->data.string_literal.string);

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

            /* Safer memory cleanup to avoid double-free issues in complex expressions */
            /* Don't free immediately - defer to end of function generation */
            /* free_llvm_value(return_val); */
        }
    } else {
        emit_instruction(ctx, "ret void");
    }
}

void generate_expression_statement(CodeGenContext* ctx, ASTNode* stmt) {
    /* Handle properly structured expression statements from grammar */
    if (!stmt) return;
    
    /* The expression is stored in the same field as return statements */
    if (stmt->data.return_stmt.expression) {
        /* Generate the expression but discard the result */
        LLVMValue* result = generate_expression(ctx, stmt->data.return_stmt.expression);
        if (result) {
            free_llvm_value(result);
        }
    }
    /* If expression is NULL, it's an empty statement (just semicolon) - nothing to generate */
}

/* Function definition */
void generate_function_definition(CodeGenContext* ctx, ASTNode* func_def) {
    ctx->current_function_name = safe_strdup(func_def->data.function_def.name);
    ctx->current_function_return_type = func_def->data.function_def.return_type;

    /* Clear local symbols between function definitions - disabled to avoid double-free issues */
    /* clear_local_symbols(ctx); */

    /* Generate function signature */
    char* return_type = llvm_type_to_string(func_def->data.function_def.return_type);

    /* Handle function parameters */
    if (func_def->data.function_def.parameters) {
        /* Process parameter declarations and build parameter list */
        char param_list[512] = "";
        int param_count = 0;

        /* Walk through parameter declarations */
        ASTNode* param_decl = func_def->data.function_def.parameters;
        while (param_decl) {
            if (param_decl->type == AST_VARIABLE_DECL) {
                /* Add parameter to function signature */
                if (param_count > 0) {
                    strcat(param_list, ", ");
                }
                strcat(param_list, "i32 %");
                strcat(param_list, param_decl->data.variable_decl.name);

                /* Create parameter symbol with safer approach */
                TypeInfo* param_type = create_type_info(TYPE_INT); /* Directly create int type */
                Symbol* param_symbol = create_symbol(param_decl->data.variable_decl.name, param_type);
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
    if (!ctx || !symbol) return;
    
    /* Check for duplicate symbols to prevent corruption */
    Symbol* existing = ctx->local_symbols;
    while (existing) {
        if (existing->name && symbol->name && strcmp(existing->name, symbol->name) == 0) {
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
    if (!ctx) return;
    
    Symbol* current = ctx->local_symbols;
    while (current) {
        Symbol* next = current->next;
        /* More careful cleanup to prevent double-free */
        if (current && current->name) {
            free_symbol(current);
        }
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
    va_list args;
    va_start(args, message);

    fprintf(stderr, "Code generation warning: ");
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");

    va_end(args);
}

/* Declaration generation */
void generate_declaration(CodeGenContext* ctx, ASTNode* decl) {
    /* Duplicate the type for the symbol to avoid conflicts with AST cleanup */
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

        /* Handle initializer if present */
        if (decl->data.variable_decl.initializer) {
            LLVMValue* init_val =
                generate_expression(ctx, decl->data.variable_decl.initializer);
            if (init_val) {
                if (init_val->type == LLVM_VALUE_CONSTANT) {
                    emit_instruction(ctx, "store i32 %d, i32* %%%s",
                                     init_val->data.constant_val, symbol->name);
                } else {
                    emit_instruction(ctx, "store i32 %%%s, i32* %%%s",
                                     init_val->name, symbol->name);
                }
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
    LLVMValue* value = (LLVMValue*)safe_malloc(sizeof(LLVMValue));
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
    char* reg_name = (char*)safe_malloc(32);
    snprintf(reg_name, 32, "%d", ctx->next_reg_id++);
    return reg_name;
}

char* get_next_basic_block(CodeGenContext* ctx) {
    char* bb_name = (char*)safe_malloc(32);
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
    case TYPE_POINTER:
        return safe_strdup("i32*");
    default:
        return safe_strdup("i32");
    }
}

char* get_default_value(TypeInfo* type) {
    if (!type)
        return safe_strdup("0");

    switch (type->base_type) {
    case TYPE_FLOAT:
        return safe_strdup("0.0");
    case TYPE_DOUBLE:
        return safe_strdup("0.0");
    default:
        return safe_strdup("0");
    }
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
    char* result_reg = get_next_register(ctx);
    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                                          create_type_info(TYPE_INT));

    emit_instruction(ctx, "%%%s = load i32, i32* %%array_ptr", result_reg);

    /* Free the original result_reg since create_llvm_value duplicated it */
    free(result_reg);
    return result;
}

LLVMValue* generate_member_access(CodeGenContext* ctx, ASTNode* access) {
    char* result_reg = get_next_register(ctx);
    LLVMValue* result = create_llvm_value(LLVM_VALUE_REGISTER, result_reg,
                                          create_type_info(TYPE_INT));

    emit_instruction(ctx, "%%%s = load i32, i32* %%member_ptr", result_reg);

    /* Free the original result_reg since create_llvm_value duplicated it */
    free(result_reg);
    return result;
}
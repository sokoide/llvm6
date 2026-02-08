#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper function to allocate memory safely */
static void* safe_malloc(size_t size) {
    auto ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(1);
    }
    return ptr;
}

/* Helper function to duplicate strings safely */
static char* safe_strdup(const char* str) {
    if (!str)
        return NULL;
    auto new_str = static_cast<char*>(safe_malloc(strlen(str) + 1));
    strcpy(new_str, str);
    return new_str;
}

/* AST Node creation functions */
ASTNode* create_ast_node(ASTNodeType type) {
    /* Use placement new with zero-initialization instead of memset to avoid
       portability issues with floating point members in unions */
    auto node = static_cast<ASTNode*>(safe_malloc(sizeof(ASTNode)));
    *node = ASTNode{}; /* Zero-initialize entire structure */
    node->type = type;
    return node;
}

ASTNode* create_identifier_node(const char* name) {
    ASTNode* node = create_ast_node(AST_IDENTIFIER);
    node->data.identifier.name = safe_strdup(name);
    node->data.identifier.symbol = NULL;
    node->data.identifier.parameters = NULL;
    return node;
}

ASTNode* create_constant_node(int value, DataType type) {
    ASTNode* node = create_ast_node(AST_CONSTANT);
    node->data.constant.value.int_val = value;
    node->data.constant.const_type = type;
    return node;
}

ASTNode* create_string_literal_node(const char* string) {
    ASTNode* node = create_ast_node(AST_STRING_LITERAL);

    size_t len = strlen(string);
    char* processed = (char*)safe_malloc(len + 1);

    size_t j = 0;
    /* Skip first quote (i=1), end before last quote (i < len-1) */
    for (size_t i = 1; i < len - 1; i++) {
        if (string[i] == '\\' && i + 1 < len - 1) {
            char c = string[i+1];
            switch (c) {
                case 'n': processed[j++] = '\n'; break;
                case 't': processed[j++] = '\t'; break;
                case 'r': processed[j++] = '\r'; break;
                case '0': processed[j++] = '\0'; break;
                case '\\': processed[j++] = '\\'; break;
                case '\'': processed[j++] = '\''; break;
                case '"': processed[j++] = '"'; break;
                default: processed[j++] = c; break;
            }
            i++;
        } else {
            processed[j++] = string[i];
        }
    }
    processed[j] = '\0';

    node->data.string_literal.string = processed;
    node->data.string_literal.length = j;
    return node;
}

int parse_constant_value(const char* s) {
    if (!s) return 0;
    if (s[0] == '\'') {
        if (s[1] == '\\') {
            switch (s[2]) {
                case 'n': return '\n';
                case 't': return '\t';
                case 'r': return '\r';
                case '0': return '\0';
                case '\\': return '\\';
                case '\'': return '\'';
                case '\"': return '\"';
                default: return s[2];
            }
        }
        return s[1];
    }
    // Handle hex, octal, decimal
    return (int)strtol(s, NULL, 0);
}

ASTNode* create_binary_op_node(BinaryOp op, ASTNode* left, ASTNode* right) {
    ASTNode* node = create_ast_node(AST_BINARY_OP);
    node->data.binary_op.op = op;
    node->data.binary_op.left = left;
    node->data.binary_op.right = right;
    return node;
}

ASTNode* create_unary_op_node(UnaryOp op, ASTNode* operand) {
    ASTNode* node = create_ast_node(AST_UNARY_OP);
    node->data.unary_op.op = op;
    node->data.unary_op.operand = operand;
    return node;
}

ASTNode* create_function_call_node(ASTNode* function, ASTNode* arguments) {
    ASTNode* node = create_ast_node(AST_FUNCTION_CALL);
    node->data.function_call.function = function;
    node->data.function_call.arguments = arguments;
    return node;
}

ASTNode* create_compound_stmt_node(ASTNode* statements) {
    ASTNode* node = create_ast_node(AST_COMPOUND_STMT);
    node->data.compound_stmt.statements = statements;
    return node;
}

ASTNode* create_if_stmt_node(ASTNode* condition, ASTNode* then_stmt,
                             ASTNode* else_stmt) {
    ASTNode* node = create_ast_node(AST_IF_STMT);
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_stmt = then_stmt;
    node->data.if_stmt.else_stmt = else_stmt;
    return node;
}

ASTNode* create_while_stmt_node(ASTNode* condition, ASTNode* body) {
    ASTNode* node = create_ast_node(AST_WHILE_STMT);
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;
    return node;
}

ASTNode* create_for_stmt_node(ASTNode* init, ASTNode* condition,
                              ASTNode* update, ASTNode* body) {
    ASTNode* node = create_ast_node(AST_FOR_STMT);
    node->data.for_stmt.init = init;
    node->data.for_stmt.condition = condition;
    node->data.for_stmt.update = update;
    node->data.for_stmt.body = body;
    return node;
}

ASTNode* create_return_stmt_node(ASTNode* expression) {
    ASTNode* node = create_ast_node(AST_RETURN_STMT);
    node->data.return_stmt.expression = expression;
    return node;
}

ASTNode* create_variable_decl_node(TypeInfo* type, const char* name,
                                   ASTNode* initializer) {
    ASTNode* node = create_ast_node(AST_VARIABLE_DECL);
    node->data.variable_decl.type = type;
    node->data.variable_decl.name = safe_strdup(name);
    node->data.variable_decl.initializer = initializer;
    return node;
}

ASTNode* create_function_decl_node(TypeInfo* return_type, const char* name,
                                   ASTNode* parameters, int is_variadic) {
    ASTNode* node = create_ast_node(AST_FUNCTION_DECL);
    node->data.function_def.return_type = return_type;
    node->data.function_def.name = safe_strdup(name);
    node->data.function_def.parameters = parameters;
    node->data.function_def.body = NULL;
    node->data.function_def.is_variadic = is_variadic;
    return node;
}

ASTNode* create_function_def_node(TypeInfo* return_type, const char* name,
                                  ASTNode* parameters, ASTNode* body,
                                  int is_variadic) {
    ASTNode* node = create_ast_node(AST_FUNCTION_DEF);
    node->data.function_def.return_type = return_type;
    node->data.function_def.name = safe_strdup(name);
    node->data.function_def.parameters = parameters;
    node->data.function_def.body = body;
    node->data.function_def.is_variadic = is_variadic;
    return node;
}

/* Type creation functions */
TypeInfo* create_type_info(DataType base_type) {
    auto type = static_cast<TypeInfo*>(safe_malloc(sizeof(TypeInfo)));
    memset(type, 0, sizeof(TypeInfo));
    type->base_type = base_type;
    type->qualifiers = QUAL_NONE;
    type->storage_class = STORAGE_NONE;
    return type;
}

TypeInfo* duplicate_type_info(TypeInfo* original) {
    if (!original)
        return NULL;

    auto copy = static_cast<TypeInfo*>(safe_malloc(sizeof(TypeInfo)));
    memcpy(copy, original, sizeof(TypeInfo));

    /* Deep copy pointer fields */
    if (original->return_type) {
        copy->return_type = duplicate_type_info(original->return_type);
    }
    if (original->parameters) {
        /* Note: We don't duplicate parameters to avoid circular dependencies */
        copy->parameters = NULL;
    }
    if (original->struct_name) {
        copy->struct_name = safe_strdup(original->struct_name);
    }
    if (original->next) {
        copy->next = duplicate_type_info(original->next);
    }

    return copy;
}

TypeInfo* create_pointer_type(TypeInfo* base_type) {
    TypeInfo* type = create_type_info(TYPE_POINTER);

    if (base_type) {
        type->return_type = duplicate_type_info(base_type);
        if (base_type->base_type == TYPE_POINTER) {
            type->pointer_level = base_type->pointer_level + 1;
        } else {
            type->pointer_level = 1;
        }
    } else {
        type->return_type = NULL;
        type->pointer_level = 1;
    }

    return type;
}

TypeInfo* create_array_type(TypeInfo* base_type, int size) {
    TypeInfo* type = create_type_info(TYPE_ARRAY);
    type->return_type = base_type;
    type->array_size = size;
    return type;
}

TypeInfo* create_function_type(TypeInfo* return_type, ASTNode* parameters) {
    TypeInfo* type = create_type_info(TYPE_FUNCTION);
    type->return_type = return_type;
    type->parameters = parameters;
    return type;
}

/* Memory cleanup functions */
void free_ast_node(ASTNode* node) {
    if (!node)
        return;

    switch (node->type) {
    case AST_IDENTIFIER:
        free(node->data.identifier.name);
        break;
    case AST_STRING_LITERAL:
        free(node->data.string_literal.string);
        break;
    case AST_BINARY_OP:
        free_ast_node(node->data.binary_op.left);
        free_ast_node(node->data.binary_op.right);
        break;
    case AST_UNARY_OP:
        free_ast_node(node->data.unary_op.operand);
        break;
    case AST_FUNCTION_CALL:
        free_ast_node(node->data.function_call.function);
        free_ast_node(node->data.function_call.arguments);
        break;
    case AST_COMPOUND_STMT:
        free_ast_node(node->data.compound_stmt.statements);
        break;
    case AST_IF_STMT:
        free_ast_node(node->data.if_stmt.condition);
        free_ast_node(node->data.if_stmt.then_stmt);
        free_ast_node(node->data.if_stmt.else_stmt);
        break;
    case AST_WHILE_STMT:
        free_ast_node(node->data.while_stmt.condition);
        free_ast_node(node->data.while_stmt.body);
        break;
    case AST_FOR_STMT:
        free_ast_node(node->data.for_stmt.init);
        free_ast_node(node->data.for_stmt.condition);
        free_ast_node(node->data.for_stmt.update);
        free_ast_node(node->data.for_stmt.body);
        break;
    case AST_RETURN_STMT:
        free_ast_node(node->data.return_stmt.expression);
        break;
    case AST_VARIABLE_DECL:
        free_type_info(node->data.variable_decl.type);
        free(node->data.variable_decl.name);
        free_ast_node(node->data.variable_decl.initializer);
        break;
    case AST_FUNCTION_DECL:
    case AST_FUNCTION_DEF:
        free_type_info(node->data.function_def.return_type);
        free(node->data.function_def.name);
        free_ast_node(node->data.function_def.parameters);
        if (node->type == AST_FUNCTION_DEF) {
            free_ast_node(node->data.function_def.body);
        }
        break;
    default:
        /* Handle other node types as needed */
        break;
    }

    if (node->data_type) {
        free_type_info(node->data_type);
    }

    /* Free next sibling if it exists */
    if (node->next) {
        free_ast_node(node->next);
    }

    free(node);
}

void free_type_info(TypeInfo* type) {
    if (!type)
        return;

    if (type->return_type) {
        free_type_info(type->return_type);
    }
    if (type->parameters) {
        free_ast_node(type->parameters);
    }
    if (type->struct_name) {
        free(type->struct_name);
    }
    if (type->next) {
        free_type_info(type->next);
    }

    free(type);
}

/* Symbol table functions */
Symbol* create_symbol(const char* name, TypeInfo* type) {
    auto symbol = static_cast<Symbol*>(safe_malloc(sizeof(Symbol)));
    symbol->name = safe_strdup(name);
    symbol->type = type; /* Use the type directly - ownership transferred */
    symbol->offset = 0;
    symbol->is_global = 0;
    symbol->is_parameter = 0;
    symbol->next = NULL;
    return symbol;
}

void free_symbol(Symbol* symbol) {
    if (!symbol)
        return;

    free(symbol->name);
    free_type_info(symbol->type);
    free(symbol);
}

/* Debugging/printing functions */
static const char* node_type_to_string(ASTNodeType type) {
    switch (type) {
    case AST_IDENTIFIER:
        return "IDENTIFIER";
    case AST_CONSTANT:
        return "CONSTANT";
    case AST_STRING_LITERAL:
        return "STRING_LITERAL";
    case AST_BINARY_OP:
        return "BINARY_OP";
    case AST_UNARY_OP:
        return "UNARY_OP";
    case AST_FUNCTION_CALL:
        return "FUNCTION_CALL";
    case AST_IF_STMT:
        return "IF_STMT";
    case AST_WHILE_STMT:
        return "WHILE_STMT";
    case AST_FOR_STMT:
        return "FOR_STMT";
    case AST_RETURN_STMT:
        return "RETURN_STMT";
    case AST_VARIABLE_DECL:
        return "VARIABLE_DECL";
    case AST_FUNCTION_DECL:
        return "FUNCTION_DECL";
    case AST_FUNCTION_DEF:
        return "FUNCTION_DEF";
    case AST_COMPOUND_STMT:
        return "COMPOUND_STMT";
    default:
        return "UNKNOWN";
    }
}

static const char* data_type_to_string(DataType type) {
    switch (type) {
    case TYPE_VOID:
        return "void";
    case TYPE_CHAR:
        return "char";
    case TYPE_SHORT:
        return "short";
    case TYPE_INT:
        return "int";
    case TYPE_LONG:
        return "long";
    case TYPE_FLOAT:
        return "float";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_POINTER:
        return "pointer";
    case TYPE_ARRAY:
        return "array";
    case TYPE_FUNCTION:
        return "function";
    default:
        return "unknown";
    }
}

void print_ast(ASTNode* node, int indent) {
    if (!node)
        return;

    for (int i = 0; i < indent; i++) {
        printf("  ");
    }

    printf("%s", node_type_to_string(node->type));

    switch (node->type) {
    case AST_IDENTIFIER:
        printf(" (%s)", node->data.identifier.name);
        break;
    case AST_CONSTANT:
        printf(" (%d)", node->data.constant.value.int_val);
        break;
    case AST_STRING_LITERAL:
        printf(" (\"%s\")", node->data.string_literal.string);
        break;
    case AST_BINARY_OP:
        printf(" (op=%d)\n", node->data.binary_op.op);
        print_ast(node->data.binary_op.left, indent + 1);
        print_ast(node->data.binary_op.right, indent + 1);
        return;
    case AST_UNARY_OP:
        printf(" (op=%d)\n", node->data.unary_op.op);
        print_ast(node->data.unary_op.operand, indent + 1);
        return;
    case AST_FUNCTION_CALL:
        printf("\n");
        print_ast(node->data.function_call.function, indent + 1);
        print_ast(node->data.function_call.arguments, indent + 1);
        return;
    case AST_VARIABLE_DECL:
        printf(" (%s)", node->data.variable_decl.name);
        break;
    case AST_FUNCTION_DECL:
    case AST_FUNCTION_DEF:
        printf(" (%s)", node->data.function_def.name);
        break;
    default:
        break;
    }

    printf("\n");

    /* Print next sibling if it exists */
    if (node->next) {
        print_ast(node->next, indent);
    }
}

void print_type_info(const TypeInfo* type) {
    if (!type) {
        printf("(null type)");
        return;
    }

    printf("%s", data_type_to_string(type->base_type));

    if (type->qualifiers & QUAL_CONST) {
        printf(" const");
    }
    if (type->qualifiers & QUAL_VOLATILE) {
        printf(" volatile");
    }
}
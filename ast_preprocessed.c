typedef void* FILE;
int printf(const char* format, ...);
int fprintf(FILE* stream, const char* format, ...);
int sprintf(char* str, const char* format, ...);
FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
int fputs(const char* s, FILE* stream);
int fflush(FILE* stream);
void* malloc(unsigned long size);
void* realloc(void* ptr, unsigned long size);
void free(void* ptr);
void* memset(void* s, int c, unsigned long n);
void* memcpy(void* dest, const void* src, unsigned long n);
int strcmp(const char* s1, const char* s2);
int atoi(const char* s);
long strtol(const char* s, char** endptr, int base);
unsigned long strlen(const char* s);
char* strdup(const char* s);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
char* strstr(const char* haystack, const char* needle);
void exit(int status);
typedef _Bool bool;
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long int64_t;
typedef unsigned long uint64_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
typedef int va_list;
typedef struct Arena Arena;
Arena* arena_create(size_t initial_capacity);
void* arena_alloc(Arena* arena, size_t size);
void arena_reset(Arena* arena);
void arena_destroy(Arena* arena);
extern Arena* g_compiler_arena;
void mem_init(void);
void mem_cleanup(void);
char* arena_strdup(Arena* arena, const char* str);
void error_report(const char* format, ...);
void fatal_error(const char* format, ...);
int error_get_count(void);
void error_suppress_output(bool suppress);
typedef struct ASTNode ASTNode;
typedef struct TypeInfo TypeInfo;
typedef struct Symbol Symbol;
typedef enum {
    AST_IDENTIFIER,
    AST_CONSTANT,
    AST_STRING_LITERAL,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_ASSIGNMENT,
    AST_FUNCTION_CALL,
    AST_ARRAY_ACCESS,
    AST_MEMBER_ACCESS,
    AST_CAST,
    AST_CONDITIONAL,
    AST_COMPOUND_STMT,
    AST_EXPRESSION_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_DO_WHILE_STMT,
    AST_SWITCH_STMT,
    AST_CASE_STMT,
    AST_DEFAULT_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_RETURN_STMT,
    AST_GOTO_STMT,
    AST_LABEL_STMT,
    AST_VARIABLE_DECL,
    AST_FUNCTION_DECL,
    AST_FUNCTION_DEF,
    AST_PARAMETER_DECL,
    AST_STRUCT_DECL,
    AST_UNION_DECL,
    AST_ENUM_DECL,
    AST_TYPEDEF_DECL,
    AST_POINTER_TYPE,
    AST_ARRAY_TYPE,
    AST_FUNCTION_TYPE,
    AST_STRUCT_TYPE,
    AST_UNION_TYPE,
    AST_ENUM_TYPE,
    AST_BASIC_TYPE,
    AST_TRANSLATION_UNIT,
    AST_INITIALIZER_LIST,
    AST_ARGUMENT_LIST,
    AST_PARAMETER_LIST,
    AST_DECLARATION_LIST,
    AST_STATEMENT_LIST
} ASTNodeType;
typedef enum {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_SHORT,
    TYPE_INT,
    TYPE_LONG,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_SIGNED,
    TYPE_UNSIGNED,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_ENUM,
    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_FUNCTION
} DataType;
typedef enum { QUAL_NONE = 0, QUAL_CONST = 1, QUAL_VOLATILE = 2 } TypeQualifier;
typedef enum {
    STORAGE_NONE,
    STORAGE_AUTO,
    STORAGE_REGISTER,
    STORAGE_STATIC,
    STORAGE_EXTERN,
    STORAGE_TYPEDEF
} StorageClass;
typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_LT, OP_GT, OP_LE, OP_GE, OP_EQ, OP_NE,
    OP_AND, OP_OR, OP_BITAND, OP_BITOR, OP_XOR,
    OP_LSHIFT, OP_RSHIFT,
    OP_ASSIGN, OP_ADD_ASSIGN, OP_SUB_ASSIGN,
    OP_MUL_ASSIGN, OP_DIV_ASSIGN, OP_MOD_ASSIGN,
    OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN,
    OP_LSHIFT_ASSIGN, OP_RSHIFT_ASSIGN,
    OP_COMMA
} BinaryOp;
typedef enum {
    UOP_PLUS, UOP_MINUS, UOP_NOT, UOP_BITNOT,
    UOP_PREINC, UOP_PREDEC, UOP_POSTINC, UOP_POSTDEC,
    UOP_ADDR, UOP_DEREF, UOP_SIZEOF
} UnaryOp;
struct TypeInfo {
    DataType base_type;
    TypeQualifier qualifiers;
    StorageClass storage_class;
    int pointer_level;
    int array_size;
    struct TypeInfo* return_type;
    struct ASTNode* parameters;
    char* struct_name;
    struct Symbol* struct_members;
    int size;
    int alignment;
    struct TypeInfo* next;
};
struct Symbol {
    char* name;
    TypeInfo* type;
    int offset;
    int index;
    int is_global;
    int is_parameter;
    int is_array;
    int is_enum_constant;
    int enum_value;
    struct Symbol* next;
};
struct ASTNode {
    ASTNodeType type;
    TypeInfo* data_type;
    union {
        struct {
            char* name;
            Symbol* symbol;
            ASTNode* parameters;
            int is_variadic;
            int pointer_level;
            int is_function_pointer;
            struct ASTNode* array_dimensions;
        } identifier;
        struct {
            union {
                int int_val;
                float float_val;
                char char_val;
            } value;
            DataType const_type;
        } constant;
        struct {
            char* string;
            int length;
        } string_literal;
        struct {
            BinaryOp op;
            ASTNode* left;
            ASTNode* right;
        } binary_op;
        struct {
            UnaryOp op;
            ASTNode* operand;
        } unary_op;
        struct {
            ASTNode* function;
            ASTNode* arguments;
        } function_call;
        struct {
            ASTNode* array;
            ASTNode* index;
        } array_access;
        struct {
            ASTNode* object;
            char* member;
            int is_pointer_access;
        } member_access;
        struct {
            TypeInfo* target_type;
            ASTNode* operand;
        } cast_expr;
        struct {
            ASTNode* condition;
            ASTNode* then_expr;
            ASTNode* else_expr;
        } conditional_expr;
        struct {
            ASTNode* statements;
            int num_statements;
        } compound_stmt;
        struct {
            ASTNode* condition;
            ASTNode* then_stmt;
            ASTNode* else_stmt;
        } if_stmt;
        struct {
            ASTNode* condition;
            ASTNode* body;
        } while_stmt;
        struct {
            ASTNode* init;
            ASTNode* condition;
            ASTNode* update;
            ASTNode* body;
        } for_stmt;
        struct {
            ASTNode* expression;
        } return_stmt;
        struct {
            ASTNode* expression;
            ASTNode* body;
        } switch_stmt;
        struct {
            ASTNode* value;
            ASTNode* statement;
        } case_stmt;
        struct {
            TypeInfo* type;
            char* name;
            ASTNode* initializer;
            ASTNode* parameters;
            int pointer_level;
            struct ASTNode* array_dimensions;
        } variable_decl;
        struct {
            TypeInfo* return_type;
            char* name;
            ASTNode* parameters;
            ASTNode* body;
            int is_variadic;
            int pointer_level;
        } function_def;
        struct {
            char* name;
            ASTNode* members;
            Symbol* symbol_table;
        } struct_decl;
        struct {
            ASTNode* items;
            int count;
        } initializer_list;
        struct {
            ASTNode** items;
            int count;
            int capacity;
        } list;
    } data;
    int line;
    int column;
    ASTNode* next;
};
ASTNode* create_ast_node(ASTNodeType type);
ASTNode* create_identifier_node(const char* name);
ASTNode* create_constant_node(int value, DataType type);
ASTNode* create_string_literal_node(const char* string);
int parse_constant_value(const char* s);
int evaluate_constant_node(ASTNode* node);
ASTNode* create_binary_op_node(BinaryOp op, ASTNode* left, ASTNode* right);
ASTNode* create_unary_op_node(UnaryOp op, ASTNode* operand);
ASTNode* create_function_call_node(ASTNode* function, ASTNode* arguments);
ASTNode* create_compound_stmt_node(ASTNode* statements);
ASTNode* create_if_stmt_node(ASTNode* condition, ASTNode* then_stmt, ASTNode* else_stmt);
ASTNode* create_while_stmt_node(ASTNode* condition, ASTNode* body);
ASTNode* create_for_stmt_node(ASTNode* init, ASTNode* condition, ASTNode* update, ASTNode* body);
ASTNode* create_return_stmt_node(ASTNode* expression);
ASTNode* create_variable_decl_node(TypeInfo* type, const char* name, ASTNode* initializer);
ASTNode* create_function_decl_node(TypeInfo* return_type, const char* name, ASTNode* parameters, int is_variadic);
ASTNode* create_function_def_node(TypeInfo* return_type, const char* name, ASTNode* parameters, ASTNode* body, int is_variadic);
TypeInfo* create_type_info(DataType base_type);
int get_type_size(TypeInfo* type);
int get_type_alignment(TypeInfo* type);
TypeInfo* create_struct_type(const char* tag, int is_union);
void struct_add_member(TypeInfo* type, const char* name, TypeInfo* member_type);
void struct_finish_layout(TypeInfo* type);
Symbol* struct_lookup_member(TypeInfo* type, const char* name);
TypeInfo* duplicate_type_info(TypeInfo* original);
TypeInfo* create_pointer_type(TypeInfo* base_type);
TypeInfo* create_array_type(TypeInfo* base_type, int size);
TypeInfo* create_function_type(TypeInfo* return_type, ASTNode* parameters);
void free_ast_node(ASTNode* node);
void free_type_info(TypeInfo* type);
void print_ast(ASTNode* node, int indent);
void print_type_info(const TypeInfo* type);
Symbol* create_symbol(const char* name, TypeInfo* type);
void free_symbol(Symbol* symbol);

ASTNode* create_ast_node(ASTNodeType type) {
    ASTNode* node = (ASTNode*)arena_alloc(g_compiler_arena, sizeof(ASTNode));
    node->type = type;
    node->data_type = NULL;
    node->next = NULL;
    node->line = 0;
    node->column = 0;
    return node;
}
ASTNode* create_identifier_node(const char* name) {
    ASTNode* node = create_ast_node(AST_IDENTIFIER);
    node->data.identifier.name = arena_strdup(g_compiler_arena, name);
    node->data.identifier.symbol = NULL;
    node->data.identifier.parameters = NULL;
    node->data.identifier.is_variadic = 0;
    node->data.identifier.pointer_level = 0;
    node->data.identifier.is_function_pointer = 0;
    node->data.identifier.array_dimensions = NULL;
    return node;
}
ASTNode* create_constant_node(int value, DataType type) {
    ASTNode* node = create_ast_node(AST_CONSTANT);
    node->data.constant.value.int_val = value;
    node->data.constant.const_type = type;
    node->data_type = create_type_info(type);
    return node;
}
ASTNode* create_string_literal_node(const char* string) {
    ASTNode* node = create_ast_node(AST_STRING_LITERAL);
    node->data.string_literal.string = arena_strdup(g_compiler_arena, string);
    node->data.string_literal.length = strlen(string);
    node->data_type = create_pointer_type(create_type_info(TYPE_CHAR));
    return node;
}
int parse_constant_value(const char* s) {
    if (!s) return 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        return (int)strtol(s, NULL, 16);
    }
    if (s[0] == '\'') {
        if (s[1] == '\\') {
            switch (s[2]) {
                case 'n': return '\n';
                case 't': return '\t';
                case 'r': return '\r';
                case '0': return '\0';
                default: return s[2];
            }
        }
        return s[1];
    }
    return atoi(s);
}
int evaluate_constant_node(ASTNode* node) {
    if (!node) return 0;
    if (node->type == AST_CONSTANT) {
        return node->data.constant.value.int_val;
    }
    if (node->type == AST_BINARY_OP) {
        int left = evaluate_constant_node(node->data.binary_op.left);
        int right = evaluate_constant_node(node->data.binary_op.right);
        switch (node->data.binary_op.op) {
            case OP_ADD: return left + right;
            case OP_SUB: return left - right;
            case OP_MUL: return left * right;
            case OP_DIV: return right != 0 ? left / right : 0;
            default: return 0;
        }
    }
    return 0;
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
    node->data.compound_stmt.num_statements = 0;
    return node;
}
ASTNode* create_if_stmt_node(ASTNode* condition, ASTNode* then_stmt, ASTNode* else_stmt) {
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
ASTNode* create_for_stmt_node(ASTNode* init, ASTNode* condition, ASTNode* update, ASTNode* body) {
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
ASTNode* create_variable_decl_node(TypeInfo* type, const char* name, ASTNode* initializer) {
    ASTNode* node = create_ast_node(AST_VARIABLE_DECL);
    node->data.variable_decl.type = type;
    node->data.variable_decl.name = arena_strdup(g_compiler_arena, name);
    node->data.variable_decl.initializer = initializer;
    node->data.variable_decl.parameters = NULL;
    node->data.variable_decl.pointer_level = 0;
    node->data.variable_decl.array_dimensions = NULL;
    return node;
}
ASTNode* create_function_decl_node(TypeInfo* return_type, const char* name, ASTNode* parameters, int is_variadic) {
    ASTNode* node = create_ast_node(AST_FUNCTION_DECL);
    node->data.function_def.return_type = return_type;
    node->data.function_def.name = arena_strdup(g_compiler_arena, name);
    node->data.function_def.parameters = parameters;
    node->data.function_def.body = NULL;
    node->data.function_def.is_variadic = is_variadic;
    node->data.function_def.pointer_level = 0;
    return node;
}
ASTNode* create_function_def_node(TypeInfo* return_type, const char* name, ASTNode* parameters, ASTNode* body, int is_variadic) {
    ASTNode* node = create_ast_node(AST_FUNCTION_DEF);
    node->data.function_def.return_type = return_type;
    node->data.function_def.name = arena_strdup(g_compiler_arena, name);
    node->data.function_def.parameters = parameters;
    node->data.function_def.body = body;
    node->data.function_def.is_variadic = is_variadic;
    node->data.function_def.pointer_level = 0;
    return node;
}
TypeInfo* create_type_info(DataType base_type) {
    TypeInfo* type = (TypeInfo*)arena_alloc(g_compiler_arena, sizeof(TypeInfo));
    type->base_type = base_type;
    type->qualifiers = QUAL_NONE;
    type->storage_class = STORAGE_NONE;
    type->pointer_level = 0;
    type->array_size = 0;
    type->return_type = NULL;
    type->parameters = NULL;
    type->struct_name = NULL;
    type->struct_members = NULL;
    type->size = 0;
    type->alignment = 0;
    type->next = NULL;
    return type;
}
TypeInfo* duplicate_type_info(TypeInfo* original) {
    if (!original) return NULL;
    TypeInfo* type = (TypeInfo*)arena_alloc(g_compiler_arena, sizeof(TypeInfo));
    memcpy(type, original, sizeof(TypeInfo));
    type->next = NULL;
    return type;
}
TypeInfo* create_pointer_type(TypeInfo* base_type) {
    TypeInfo* type = duplicate_type_info(base_type);
    type->pointer_level++;
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
void free_ast_node(ASTNode* node) {
    (void)node;
}
void free_type_info(TypeInfo* type) {
    (void)type;
}
static const char* data_type_to_string(DataType type) {
    switch (type) {
        case TYPE_VOID: return "void";
        case TYPE_BOOL: return "bool";
        case TYPE_CHAR: return "char";
        case TYPE_SHORT: return "short";
        case TYPE_INT: return "int";
        case TYPE_LONG: return "long";
        case TYPE_FLOAT: return "float";
        case TYPE_DOUBLE: return "double";
        case TYPE_SIGNED: return "signed";
        case TYPE_UNSIGNED: return "unsigned";
        case TYPE_STRUCT: return "struct";
        case TYPE_UNION: return "union";
        case TYPE_ENUM: return "enum";
        case TYPE_POINTER: return "pointer";
        case TYPE_ARRAY: return "array";
        case TYPE_FUNCTION: return "function";
        default: return "unknown";
    }
}
void print_ast(ASTNode* node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; i++) printf("  ");
    switch (node->type) {
        case AST_IDENTIFIER:
            printf("Identifier: %s\n", node->data.identifier.name);
            break;
        case AST_CONSTANT:
            printf("Constant: %d\n", node->data.constant.value.int_val);
            break;
        case AST_VARIABLE_DECL:
            printf("Variable Decl: %s\n", node->data.variable_decl.name);
            break;
        case AST_FUNCTION_DEF:
            printf("Function Def: %s\n", node->data.function_def.name);
            print_ast(node->data.function_def.body, indent + 1);
            break;
        case AST_BINARY_OP:
            printf("Binary Op: %d\n", node->data.binary_op.op);
            print_ast(node->data.binary_op.left, indent + 1);
            print_ast(node->data.binary_op.right, indent + 1);
            break;
        case AST_RETURN_STMT:
            printf("Return\n");
            print_ast(node->data.return_stmt.expression, indent + 1);
            break;
        default:
            printf("Node type: %d\n", node->type);
            break;
    }
    if (node->next) print_ast(node->next, indent);
}
void print_type_info(const TypeInfo* type) {
    if (!type) { printf("(null type)"); return; }
    printf("%s", data_type_to_string(type->base_type));
    if (type->qualifiers & QUAL_CONST) printf(" const");
    if (type->qualifiers & QUAL_VOLATILE) printf(" volatile");
}
Symbol* create_symbol(const char* name, TypeInfo* type) {
    Symbol* symbol = (Symbol*)arena_alloc(g_compiler_arena, sizeof(Symbol));
    symbol->name = arena_strdup(g_compiler_arena, name);
    symbol->type = type;
    symbol->offset = 0;
    symbol->is_global = 0;
    symbol->is_parameter = 0;
    symbol->is_array = 0;
    symbol->is_enum_constant = 0;
    symbol->enum_value = 0;
    symbol->next = NULL;
    return symbol;
}
int get_type_size(TypeInfo* type) {
    if (!type) return 0;
    if (type->pointer_level > 0) return 8;
    switch (type->base_type) {
        case TYPE_VOID: return 0;
        case TYPE_BOOL: return 1;
        case TYPE_CHAR: return 1;
        case TYPE_SHORT: return 2;
        case TYPE_INT: return 4;
        case TYPE_LONG: return 8;
        case TYPE_FLOAT: return 4;
        case TYPE_DOUBLE: return 8;
        case TYPE_STRUCT: return type->size;
        case TYPE_UNION: return type->size;
        case TYPE_ENUM: return 4;
        case TYPE_ARRAY: return get_type_size(type->return_type) * type->array_size;
        default: return 0;
    }
}
int get_type_alignment(TypeInfo* type) {
    if (!type) return 1;
    if (type->pointer_level > 0) return 8;
    switch (type->base_type) {
        case TYPE_BOOL: return 1;
        case TYPE_CHAR: return 1;
        case TYPE_SHORT: return 2;
        case TYPE_INT: return 4;
        case TYPE_LONG: return 8;
        case TYPE_FLOAT: return 4;
        case TYPE_DOUBLE: return 8;
        case TYPE_STRUCT: return type->alignment;
        case TYPE_UNION: return type->alignment;
        case TYPE_ENUM: return 4;
        case TYPE_ARRAY: return get_type_alignment(type->return_type);
        default: return 1;
    }
}
static int g_anon_type_id = 0;
TypeInfo* create_struct_type(const char* tag, int is_union) {
    TypeInfo* type = create_type_info(is_union ? TYPE_UNION : TYPE_STRUCT);
    if (tag) {
        type->struct_name = arena_strdup(g_compiler_arena, tag);
    } else {
        char buf[32];
        sprintf(buf, "anon.%d", g_anon_type_id++);
        type->struct_name = arena_strdup(g_compiler_arena, buf);
    }
    return type;
}
void struct_add_member(TypeInfo* type, const char* name, TypeInfo* member_type) {
    Symbol* member = create_symbol(name, member_type);
    if (!type->struct_members) {
        type->struct_members = member;
    } else {
        Symbol* curr = type->struct_members;
        while (curr->next) curr = curr->next;
        curr->next = member;
    }
}
void struct_finish_layout(TypeInfo* type) {
    int current_offset = 0;
    int max_alignment = 1;
    int index = 0;
    Symbol* curr = type->struct_members;
    while (curr) {
        int sz = get_type_size(curr->type);
        int al = get_type_alignment(curr->type);
        if (al > max_alignment) max_alignment = al;
        curr->index = index++;
        if (type->base_type == TYPE_STRUCT) {
            current_offset = (current_offset + al - 1) & ~(al - 1);
            curr->offset = current_offset;
            current_offset += sz;
        } else {
            curr->offset = 0;
            if (sz > current_offset) current_offset = sz;
        }
        curr = curr->next;
    }
    type->alignment = max_alignment;
    type->size = (current_offset + max_alignment - 1) & ~(max_alignment - 1);
}
Symbol* struct_lookup_member(TypeInfo* type, const char* name) {
    if (!type || (type->base_type != TYPE_STRUCT && type->base_type != TYPE_UNION)) return NULL;
    Symbol* m = type->struct_members;
    while (m) {
        if (strcmp(m->name, name) == 0) return m;
        m = m->next;
    }
    return NULL;
}

// Simple unit test without external dependencies
#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <getopt.h>
#include <unistd.h>

extern "C" {
    #include "../../src/ast.h"
    #include "../../src/error_handling.h"
    #include "../../src/memory_management.h"
    #include "../../src/codegen.h"
    #include "../../src/constants.h"
}

/* Forward declarations from main.cpp to exercise CLI helpers */
extern FILE* yyin;
extern ASTNode* program_ast;

struct CompilerOptions {
    char* input_file;
    char* output_file;
    int debug_mode;
    int verbose;
    int dump_ast;
    int dump_tokens;
};

extern CompilerOptions options;
void print_usage(const char* program_name);
int parse_arguments(int argc, char* argv[]);
void cleanup_resources(void);
void compiler_info(void);
void debug_print(const char* format, ...);
void verbose_print(const char* format, ...);
extern int optind;
extern int opterr;
int ccompiler_main(int argc, char* argv[]);

int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    void test_##name(); \
    void run_test_##name() { \
        std::cout << "Testing " << #name << "... "; \
        try { \
            test_##name(); \
            std::cout << "PASSED\n"; \
            tests_passed++; \
        } catch (...) { \
            std::cout << "FAILED\n"; \
            tests_failed++; \
        } \
    } \
    void test_##name()

#define ASSERT(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "Assertion failed: " << #expr << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            throw std::runtime_error("Assertion failed"); \
        } \
    } while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_NULL(ptr) ASSERT((ptr) == nullptr)
#define ASSERT_NOT_NULL(ptr) ASSERT((ptr) != nullptr)
#define ASSERT_STREQ(a, b) ASSERT(strcmp((a), (b)) == 0)

// Helper utilities for constructing test ASTs and capturing IR output
static ASTNode* make_expression_stmt(ASTNode* expression) {
    ASTNode* stmt = create_ast_node(AST_EXPRESSION_STMT);
    stmt->data.return_stmt.expression = expression;
    stmt->next = nullptr;
    return stmt;
}

static std::string read_tmp_file(FILE* file) {
    fflush(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0) {
        return std::string();
    }

    std::string buffer;
    buffer.resize(static_cast<size_t>(size));
    size_t read_bytes = fread(&buffer[0], 1, static_cast<size_t>(size), file);
    buffer.resize(read_bytes);
    return buffer;
}

static void append_statement(ASTNode*& tail, ASTNode* stmt) {
    tail->next = stmt;
    tail = stmt;
}

static char* literal(const char* value) {
    return const_cast<char*>(value);
}

static void reset_compiler_options() {
    options.input_file = NULL;
    options.output_file = NULL;
    options.debug_mode = 0;
    options.verbose = 0;
    options.dump_ast = 0;
    options.dump_tokens = 0;
    optind = 1;
    opterr = 0;
}

static ASTNode* build_stub_function(const char* name, int return_value) {
    ASTNode* return_stmt = create_return_stmt_node(create_constant_node(return_value, TYPE_INT));
    ASTNode* body = create_compound_stmt_node(return_stmt);
    ASTNode* function_def = create_function_def_node(create_type_info(TYPE_INT), literal(name), NULL, body);
    function_def->next = NULL;
    return function_def;
}

// AST Tests
TEST(ast_node_creation) {
    ASTNode* node = create_ast_node(AST_IDENTIFIER);
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(node->type, AST_IDENTIFIER);
    free_ast_node(node);
}

TEST(identifier_node_creation) {
    char name[] = "variable_name";
    ASTNode* node = create_identifier_node(name);
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(node->type, AST_IDENTIFIER);
    ASSERT_NOT_NULL(node->data.identifier.name);
    ASSERT_STREQ(node->data.identifier.name, name);
    free_ast_node(node);
}

TEST(constant_node_creation) {
    ASTNode* node = create_constant_node(42, TYPE_INT);
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(node->type, AST_CONSTANT);
    ASSERT_EQ(node->data.constant.const_type, TYPE_INT);
    ASSERT_EQ(node->data.constant.value.int_val, 42);
    free_ast_node(node);
}

TEST(string_literal_node_creation) {
    char str[] = "Hello, World!";
    ASTNode* node = create_string_literal_node(str);
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(node->type, AST_STRING_LITERAL);
    ASSERT_NOT_NULL(node->data.string_literal.string);
    ASSERT_STREQ(node->data.string_literal.string, str);
    ASSERT_EQ(node->data.string_literal.length, (int)strlen(str));
    free_ast_node(node);
}

TEST(binary_op_node_creation) {
    ASTNode* left = create_constant_node(5, TYPE_INT);
    ASTNode* right = create_constant_node(3, TYPE_INT);
    ASTNode* node = create_binary_op_node(OP_ADD, left, right);
    
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(node->type, AST_BINARY_OP);
    ASSERT_EQ(node->data.binary_op.op, OP_ADD);
    ASSERT_EQ(node->data.binary_op.left, left);
    ASSERT_EQ(node->data.binary_op.right, right);
    
    free_ast_node(node);
}

TEST(type_info_creation) {
    TypeInfo* type = create_type_info(TYPE_INT);
    ASSERT_NOT_NULL(type);
    ASSERT_EQ(type->base_type, TYPE_INT);
    ASSERT_EQ(type->qualifiers, QUAL_NONE);
    ASSERT_EQ(type->storage_class, STORAGE_NONE);
    ASSERT_EQ(type->pointer_level, 0);
    free_type_info(type);
}

TEST(pointer_type_creation) {
    TypeInfo* base = create_type_info(TYPE_INT);
    TypeInfo* ptr = create_pointer_type(base);
    ASSERT_NOT_NULL(ptr);
    ASSERT_EQ(ptr->base_type, TYPE_POINTER);
    ASSERT_NOT_NULL(ptr->return_type);
    ASSERT_EQ(ptr->return_type->base_type, TYPE_INT);
    ASSERT_EQ(ptr->pointer_level, 1);
    free_type_info(base);
    free_type_info(ptr);
}

TEST(symbol_creation) {
    TypeInfo* type = create_type_info(TYPE_INT);
    char name[] = "test_var";
    Symbol* symbol = create_symbol(name, type);
    
    ASSERT_NOT_NULL(symbol);
    ASSERT_NOT_NULL(symbol->name);
    ASSERT_STREQ(symbol->name, name);
    ASSERT_EQ(symbol->type, type);
    ASSERT_EQ(symbol->is_global, false);
    ASSERT_EQ(symbol->is_parameter, false);
    
    free_symbol(symbol);
}

TEST(parse_arguments_flags) {
    reset_compiler_options();
    char prog[] = "ccompiler";
    char flags[] = "-dvta";
    char output_flag[] = "-o";
    char output_file[] = "out.ll";
    char input_file[] = "input.c";
    char* argv[] = {prog, flags, output_flag, output_file, input_file};
    int argc = sizeof(argv) / sizeof(argv[0]);

    int result = parse_arguments(argc, argv);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(options.debug_mode, 1);
    ASSERT_EQ(options.verbose, 1);
    ASSERT_EQ(options.dump_ast, 1);
    ASSERT_EQ(options.dump_tokens, 1);
    ASSERT_STREQ(options.output_file, output_file);
    ASSERT_STREQ(options.input_file, input_file);
}

TEST(parse_arguments_invalid_option) {
    reset_compiler_options();
    char prog[] = "ccompiler";
    char bad_flag[] = "--unknown";
    char* argv[] = {prog, bad_flag};

    int result = parse_arguments(2, argv);
    ASSERT_EQ(result, -1);
}

TEST(ccompiler_main_happy_path) {
    reset_compiler_options();
    yyin = NULL;

    program_ast = build_stub_function("stub", 1);

    char prog[] = "ccompiler";
    char output_flag[] = "-o";
    char output_file[] = "unit_main.ll";
    char* argv[] = {prog, output_flag, output_file};

    std::remove(output_file);
    int result = ccompiler_main(3, argv);
    ASSERT_EQ(result, 0);

    FILE* produced = fopen(output_file, "r");
    ASSERT_NOT_NULL(produced);
    char buffer[128];
    ASSERT(fgets(buffer, sizeof(buffer), produced) != nullptr);
    fclose(produced);
    std::remove(output_file);

    if (program_ast) {
        free_ast_node(program_ast);
        program_ast = NULL;
    }
    reset_compiler_options();
}

TEST(ccompiler_main_verbose_modes) {
    reset_compiler_options();
    yyin = NULL;
    program_ast = build_stub_function("verbose_stub", 2);

    char prog[] = "ccompiler";
    char flag_verbose[] = "-v";
    char flag_ast[] = "-a";
    char flag_tokens[] = "-t";
    char output_flag[] = "-o";
    char output_file[] = "unit_verbose.ll";
    char* argv[] = {prog, flag_verbose, flag_ast, flag_tokens, output_flag, output_file};

    std::remove(output_file);
    int result = ccompiler_main(6, argv);
    ASSERT_EQ(result, 0);

    FILE* produced = fopen(output_file, "r");
    ASSERT_NOT_NULL(produced);
    fclose(produced);
    std::remove(output_file);

    if (program_ast) {
        free_ast_node(program_ast);
        program_ast = NULL;
    }
    reset_compiler_options();
}

TEST(ccompiler_main_input_error) {
    reset_compiler_options();
    yyin = NULL;
    program_ast = NULL;

    char prog[] = "ccompiler";
    char missing_file[] = "__missing_input.c";
    char* argv[] = {prog, missing_file};

    int result = ccompiler_main(2, argv);
    ASSERT_EQ(result, 1);
    reset_compiler_options();
}

TEST(ccompiler_main_output_error) {
    reset_compiler_options();
    yyin = NULL;
    program_ast = NULL;

    char prog[] = "ccompiler";
    char output_flag[] = "-o";
    char invalid_path[] = "."; /* fopen on directory should fail */
    char* argv[] = {prog, output_flag, invalid_path};

    int result = ccompiler_main(3, argv);
    ASSERT_EQ(result, 1);
    reset_compiler_options();
}

TEST(run_ccompiler_on_fixture) {
    if (access("./ccompiler", X_OK) != 0) {
        return;
    }

    const char* fixtures[] = {
        "arithmetic.c",
        "arrays_pointers.c",
        "both_test.c",
        "call_test.c",
        "complex.c",
        "constants.c",
        "control_flow.c",
        "debug.c",
        "debug_func.c",
        "debug_minimal.c",
        "debug_two.c",
        "def_test.c",
        "expressions.c",
        "function.c",
        "functions.c",
        "main_only.c",
        "mixed_test.c",
        "multiple_functions.c",
        "recursive.c",
        "simple.c",
        "simple_test.c",
        "statements.c",
        "test_constant.c",
        "test_function.c",
        "test_main_empty.c",
        "test_main_return.c",
        "test_main_return_const.c",
        "test_minimal.c",
        "test_old_style.c",
        "test_simple.c",
        "test_simple_main.c",
        "two_funcs.c",
        "types.c",
        "variables.c"
    };

    for (const char* fixture : fixtures) {
        std::string command = std::string("./ccompiler tests/fixtures/") + fixture + " -o unit_fixture.ll";
        int rc = system(command.c_str());
        ASSERT_EQ(rc, 0);
        std::remove("unit_fixture.ll");
    }
}

TEST(main_print_helpers) {
    reset_compiler_options();
    options.debug_mode = 1;
    options.verbose = 1;

    print_usage("ccompiler");
    compiler_info();
    debug_print("toggled %d", options.debug_mode);
    verbose_print("output %s", "demo");
}

TEST(cleanup_resources_handles_state) {
    program_ast = create_identifier_node(literal("temp"));
    yyin = tmpfile();
    cleanup_resources();
    ASSERT_NULL(program_ast);
    ASSERT_NULL(yyin);
}

// Error Handling Tests
TEST(error_creation) {
    ErrorContext* error = create_error(ERROR_PARSE, "test.c", 42, "test_function", "Test error message");
    
    ASSERT_NOT_NULL(error);
    ASSERT_EQ(error->type, ERROR_PARSE);
    ASSERT_STREQ(error->file, "test.c");
    ASSERT_EQ(error->line, 42);
    ASSERT_STREQ(error->function, "test_function");
    ASSERT_NOT_NULL(error->message);
    ASSERT_STREQ(error->message, "Test error message");

    print_error(error);

    free_error(error);
}

TEST(error_type_to_string_covers_all) {
    ErrorType types[] = {
        ERROR_NONE,
        ERROR_MEMORY,
        ERROR_IO,
        ERROR_PARSE,
        ERROR_CODEGEN,
        ERROR_INVALID_ARGUMENT,
        ERROR_SYMBOL_NOT_FOUND,
        ERROR_TYPE_MISMATCH,
        ERROR_UNSUPPORTED_OPERATION,
        (ErrorType)999
    };

    for (ErrorType type : types) {
        const char* label = error_type_to_string(type);
        ASSERT_NOT_NULL(label);
    }
}

// Memory Management Tests
TEST(memory_context_creation) {
    MemoryContext* ctx = create_memory_context();
    ASSERT_NOT_NULL(ctx);
    ASSERT_EQ(ctx->stats.allocations, 0);
    ASSERT_EQ(ctx->stats.deallocations, 0);
    ASSERT_EQ(ctx->debug_mode, 0);
    free_memory_context(ctx);
}

TEST(safe_malloc_debug) {
    void* ptr = safe_malloc_debug(100, "test.c", 42, "test_function");
    ASSERT_NOT_NULL(ptr);
    safe_free_debug(ptr, "test.c", 43, "test_function");
}

// CodeGen Utility Tests
TEST(codegen_context_creation) {
    FILE* output = tmpfile();
    CodeGenContext* ctx = create_codegen_context(output);
    ASSERT_NOT_NULL(ctx);
    ASSERT_EQ(ctx->next_reg_id, 1);
    ASSERT_EQ(ctx->next_bb_id, 1);
    free_codegen_context(ctx);
    fclose(output);
}

TEST(register_generation) {
    FILE* output = tmpfile();
    CodeGenContext* ctx = create_codegen_context(output);
    
    char* reg1 = get_next_register(ctx);
    char* reg2 = get_next_register(ctx);
    
    ASSERT_NOT_NULL(reg1);
    ASSERT_NOT_NULL(reg2);
    ASSERT_STREQ(reg1, "1");
    ASSERT_STREQ(reg2, "2");
    
    free(reg1);
    free(reg2);
    free_codegen_context(ctx);
    fclose(output);
}

TEST(llvm_type_to_string_conversion) {
    TypeInfo* int_type = create_type_info(TYPE_INT);
    TypeInfo* float_type = create_type_info(TYPE_FLOAT);
    
    char* int_str = llvm_type_to_string(int_type);
    char* float_str = llvm_type_to_string(float_type);
    char* null_str = llvm_type_to_string(nullptr);
    
    ASSERT_STREQ(int_str, "i32");
    ASSERT_STREQ(float_str, "float");
    ASSERT_STREQ(null_str, "void");
    
    free(int_str);
    free(float_str);
    free(null_str);
    free_type_info(int_type);
    free_type_info(float_type);
}

TEST(memory_calloc_and_strdup) {
    CLEANUP_MEMORY_MANAGEMENT();
    INIT_MEMORY_MANAGEMENT();
    MemoryContext* ctx = g_memory_context;
    ASSERT_NOT_NULL(ctx);

    /* Allocate with custom metadata to test tracked free */
    char* buffer = (char*)safe_calloc_debug(8, sizeof(char), "unit", 100, __func__);
    ASSERT_NOT_NULL(buffer);
    for (int i = 0; i < 8; ++i) {
        ASSERT(buffer[i] == '\0');
    }

    char* duplicated = safe_strdup_debug("hello", "unit", 101, __func__);
    ASSERT_NOT_NULL(duplicated);
    ASSERT_STREQ(duplicated, "hello");

    safe_free_debug(buffer, "unit", 100, __func__);
    safe_free_debug(duplicated, "unit", 101, __func__);

    CLEANUP_MEMORY_MANAGEMENT();
}

TEST(memory_debug_tracking) {
    CLEANUP_MEMORY_MANAGEMENT();
    INIT_MEMORY_MANAGEMENT();
    MemoryContext* ctx = g_memory_context;
    ASSERT_NOT_NULL(ctx);

    enable_memory_debugging(ctx);
    size_t initial_allocs = ctx->stats.allocations;
    void* ptr = safe_malloc_debug(32, "unit", 120, __func__);
    ASSERT_NOT_NULL(ptr);
    ASSERT_EQ(ctx->stats.allocations, initial_allocs + 1);
    ASSERT_EQ(ctx->stats.current_usage >= 32, true);

    print_memory_stats(ctx);
    print_memory_leaks(ctx);

    safe_free_debug(ptr, "unit", 120, __func__);
    ASSERT_EQ(ctx->stats.deallocations, ctx->stats.allocations);
    print_memory_leaks(ctx);

    disable_memory_debugging(ctx);
    CLEANUP_MEMORY_MANAGEMENT();
}

TEST(codegen_return_variants) {
    FILE* output = tmpfile();
    ASSERT_NOT_NULL(output);
    CodeGenContext* ctx = create_codegen_context(output);
    ASSERT_NOT_NULL(ctx);

    ASTNode* constant_return = create_return_stmt_node(create_constant_node(7, TYPE_INT));
    generate_return_statement(ctx, constant_return);
    free_ast_node(constant_return);

    ASTNode* void_return = create_ast_node(AST_RETURN_STMT);
    void_return->data.return_stmt.expression = NULL;
    generate_return_statement(ctx, void_return);
    free_ast_node(void_return);

    std::string ir = read_tmp_file(output);
    ASSERT(ir.find("ret i32 7") != std::string::npos);
    ASSERT(ir.find("ret void") != std::string::npos);

    free_codegen_context(ctx);
    fclose(output);
}

// Additional tests for increased coverage

TEST(print_usage_function) {
    // Test print_usage function
    FILE* original_stdout = stdout;
    FILE* temp = tmpfile();
    stdout = temp;
    
    print_usage("test_compiler");
    
    stdout = original_stdout;
    
    fseek(temp, 0, SEEK_END);
    long size = ftell(temp);
    fclose(temp);
    
    if (size <= 0) {
        throw std::runtime_error("print_usage produced no output");
    }
}

TEST(parse_arguments_debug_option) {
    char* argv[] = {(char*)"test", (char*)"-d"};
    
    memset(&options, 0, sizeof(options));
    optind = 1;
    
    int result = parse_arguments(2, argv);
    (void)result;
    
    if (options.debug_mode != 1) {
        throw std::runtime_error("Debug option not set correctly");
    }
}

TEST(parse_arguments_verbose_option) {
    char* argv[] = {(char*)"test", (char*)"-v"};
    
    memset(&options, 0, sizeof(options));
    optind = 1;
    
    int result = parse_arguments(2, argv);
    (void)result;
    
    if (options.verbose != 1) {
        throw std::runtime_error("Verbose option not set correctly");
    }
}

TEST(parse_arguments_dump_ast_option) {
    char* argv[] = {(char*)"test", (char*)"-a"};
    
    memset(&options, 0, sizeof(options));
    optind = 1;
    
    int result = parse_arguments(2, argv);
    (void)result;
    
    if (options.dump_ast != 1) {
        throw std::runtime_error("Dump AST option not set correctly");
    }
}

TEST(debug_and_verbose_print_functions) {
    // Test debug print when debug mode is off
    options.debug_mode = 0;
    debug_print("test debug message");
    
    // Test debug print when debug mode is on  
    options.debug_mode = 1;
    debug_print("test debug message with debug on");
    
    // Test verbose print when verbose mode is off
    options.verbose = 0;
    verbose_print("test verbose message");
    
    // Test verbose print when verbose mode is on
    options.verbose = 1;
    verbose_print("test verbose message with verbose on");
    
    // Reset for safety
    options.debug_mode = 0;
    options.verbose = 0;
}

TEST(compiler_info_function) {
    FILE* original_stdout = stdout;
    FILE* temp = tmpfile();
    stdout = temp;
    
    compiler_info();
    
    stdout = original_stdout;
    
    fseek(temp, 0, SEEK_END);
    long size = ftell(temp);
    fclose(temp);
    
    if (size <= 0) {
        throw std::runtime_error("compiler_info produced no output");
    }
}

TEST(create_array_type) {
    TypeInfo* base = create_type_info(TYPE_INT);
    TypeInfo* array = create_array_type(base, 10);
    
    if (!array || array->base_type != TYPE_ARRAY || 
        array->array_size != 10 || array->return_type != base) {
        throw std::runtime_error("Array type creation failed");
    }
    
    free_type_info(array);
}

TEST(duplicate_type_info_with_return_type) {
    TypeInfo* original = create_type_info(TYPE_POINTER);
    original->return_type = create_type_info(TYPE_INT);
    
    TypeInfo* copy = duplicate_type_info(original);
    
    if (!copy || copy->base_type != TYPE_POINTER || 
        !copy->return_type || copy->return_type->base_type != TYPE_INT) {
        throw std::runtime_error("Type duplication with return type failed");
    }
    
    free_type_info(copy);
    free_type_info(original);
}

TEST(ast_conditional_statements) {
    // Test if/while/for statement creation and cleanup
    ASTNode* if_stmt = create_ast_node(AST_IF_STMT);
    if_stmt->data.if_stmt.condition = create_constant_node(1, TYPE_INT);
    if_stmt->data.if_stmt.then_stmt = create_ast_node(AST_COMPOUND_STMT);
    if_stmt->data.if_stmt.else_stmt = create_ast_node(AST_COMPOUND_STMT);
    
    ASTNode* while_stmt = create_ast_node(AST_WHILE_STMT);
    while_stmt->data.while_stmt.condition = create_constant_node(1, TYPE_INT);
    while_stmt->data.while_stmt.body = create_ast_node(AST_COMPOUND_STMT);
    
    ASTNode* for_stmt = create_ast_node(AST_FOR_STMT);
    for_stmt->data.for_stmt.init = create_ast_node(AST_VARIABLE_DECL);
    for_stmt->data.for_stmt.condition = create_constant_node(1, TYPE_INT);
    for_stmt->data.for_stmt.update = create_ast_node(AST_ASSIGNMENT);
    for_stmt->data.for_stmt.body = create_ast_node(AST_COMPOUND_STMT);
    
    if (!if_stmt || !while_stmt || !for_stmt) {
        throw std::runtime_error("Conditional statement creation failed");
    }
    
    free_ast_node(if_stmt);
    free_ast_node(while_stmt);
    free_ast_node(for_stmt);
}

TEST(ast_additional_node_types) {
    // Test various AST node types that might not be covered
    ASTNode* array_access = create_ast_node(AST_ARRAY_ACCESS);
    ASTNode* member_access = create_ast_node(AST_MEMBER_ACCESS);
    ASTNode* cast_expr = create_ast_node(AST_CAST);
    ASTNode* switch_stmt = create_ast_node(AST_SWITCH_STMT);
    
    if (!array_access || array_access->type != AST_ARRAY_ACCESS ||
        !member_access || member_access->type != AST_MEMBER_ACCESS ||
        !cast_expr || cast_expr->type != AST_CAST ||
        !switch_stmt || switch_stmt->type != AST_SWITCH_STMT) {
        throw std::runtime_error("AST node type creation failed");
    }
    
    free_ast_node(array_access);
    free_ast_node(member_access);
    free_ast_node(cast_expr);
    free_ast_node(switch_stmt);
}

TEST(codegen_generate_full_program) {
    FILE* output = tmpfile();
    ASSERT_NOT_NULL(output);
    CodeGenContext* ctx = create_codegen_context(output);
    ASSERT_NOT_NULL(ctx);

    /* Global declaration */
    ASTNode* global_decl = create_variable_decl_node(create_type_info(TYPE_INT), literal("global_counter"),
                                                     create_constant_node(0, TYPE_INT));

    /* Function body construction */
    ASTNode* local_decl = create_variable_decl_node(create_type_info(TYPE_INT), literal("x"),
                                                    create_constant_node(1, TYPE_INT));

    ASTNode* self_assign = make_expression_stmt(
        create_binary_op_node(OP_ASSIGN,
                              create_identifier_node(literal("x")),
                              create_identifier_node(literal("x"))));

    ASTNode* increment_assign = make_expression_stmt(
        create_binary_op_node(OP_ASSIGN,
                              create_identifier_node(literal("x")),
                              create_binary_op_node(OP_ADD,
                                                     create_identifier_node(literal("x")),
                                                     create_constant_node(1, TYPE_INT))));

    ASTNode* compound_assign = make_expression_stmt(
        create_binary_op_node(OP_ADD_ASSIGN,
                              create_identifier_node(literal("x")),
                              create_constant_node(3, TYPE_INT)));

    ASTNode* global_update = make_expression_stmt(
        create_binary_op_node(OP_ASSIGN,
                              create_identifier_node(literal("global_counter")),
                              create_binary_op_node(OP_ADD,
                                                     create_identifier_node(literal("global_counter")),
                                                     create_constant_node(1, TYPE_INT))));

    ASTNode* if_stmt = create_if_stmt_node(
        create_binary_op_node(OP_GT, create_identifier_node(literal("x")), create_constant_node(0, TYPE_INT)),
        make_expression_stmt(create_binary_op_node(OP_ASSIGN,
                                                  create_identifier_node(literal("x")),
                                                  create_binary_op_node(OP_ADD,
                                                                         create_identifier_node(literal("x")),
                                                                         create_constant_node(2, TYPE_INT)))),
        make_expression_stmt(create_binary_op_node(OP_ASSIGN,
                                                  create_identifier_node(literal("x")),
                                                  create_binary_op_node(OP_SUB,
                                                                         create_identifier_node(literal("x")),
                                                                         create_constant_node(2, TYPE_INT)))));

    ASTNode* while_stmt = create_while_stmt_node(
        create_binary_op_node(OP_GT, create_identifier_node(literal("x")), create_constant_node(0, TYPE_INT)),
        make_expression_stmt(create_binary_op_node(OP_ASSIGN,
                                                  create_identifier_node(literal("x")),
                                                  create_binary_op_node(OP_SUB,
                                                                         create_identifier_node(literal("x")),
                                                                         create_constant_node(1, TYPE_INT)))));

    ASTNode* for_cond = make_expression_stmt(
        create_binary_op_node(OP_LT, create_identifier_node(literal("x")), create_constant_node(5, TYPE_INT)));

    ASTNode* for_update = make_expression_stmt(
        create_binary_op_node(OP_ADD_ASSIGN,
                              create_identifier_node(literal("x")),
                              create_constant_node(1, TYPE_INT)));

    ASTNode* for_body_stmt = make_expression_stmt(
        create_binary_op_node(OP_ASSIGN,
                              create_identifier_node(literal("x")),
                              create_binary_op_node(OP_ADD,
                                                     create_identifier_node(literal("x")),
                                                     create_identifier_node(literal("global_counter")))));
    ASTNode* for_body = create_compound_stmt_node(for_body_stmt);
    ASTNode* for_stmt = create_for_stmt_node(NULL, for_cond, for_update, for_body);

    ASTNode* return_stmt = create_return_stmt_node(create_identifier_node(literal("x")));

    /* Chain statements inside compound block */
    ASTNode* tail = local_decl;
    append_statement(tail, self_assign);
    append_statement(tail, increment_assign);
    append_statement(tail, compound_assign);
    append_statement(tail, global_update);
    append_statement(tail, if_stmt);
    append_statement(tail, while_stmt);
    append_statement(tail, for_stmt);
    append_statement(tail, return_stmt);
    tail->next = NULL;

    ASTNode* function_body = create_compound_stmt_node(local_decl);
    ASTNode* function_def = create_function_def_node(create_type_info(TYPE_INT),
                                                     literal("exercise_features"), NULL,
                                                     function_body);

    global_decl->next = function_def;
    function_def->next = NULL;

    generate_llvm_ir(ctx, global_decl);

    std::string ir = read_tmp_file(output);
    ASSERT(ir.find("@global_counter = global i32 0") != std::string::npos);
    ASSERT(ir.find("define i32 @exercise_features()") != std::string::npos);
    ASSERT(ir.find("Runtime function declarations") != std::string::npos);
    ASSERT(ir.find("br i1") != std::string::npos);
    ASSERT(ir.find("store i32") != std::string::npos);

    free_codegen_context(ctx);
    fclose(output);
    free_ast_node(global_decl);
}

// Test runner
int main() {
    std::cout << "Running Unit Tests...\n";
    std::cout << "=====================\n\n";
    
    // AST Tests
    run_test_ast_node_creation();
    run_test_identifier_node_creation();
    run_test_constant_node_creation();
    run_test_string_literal_node_creation();
    run_test_binary_op_node_creation();
    run_test_type_info_creation();
    run_test_pointer_type_creation();
    run_test_symbol_creation();
    run_test_parse_arguments_flags();
    run_test_parse_arguments_invalid_option();
    run_test_ccompiler_main_happy_path();
    run_test_ccompiler_main_verbose_modes();
    run_test_ccompiler_main_input_error();
    run_test_ccompiler_main_output_error();
    run_test_run_ccompiler_on_fixture();
    run_test_main_print_helpers();
    run_test_cleanup_resources_handles_state();
    
    // Error Handling Tests
    run_test_error_creation();
    run_test_error_type_to_string_covers_all();
    
    // Memory Management Tests
    run_test_memory_context_creation();
    run_test_safe_malloc_debug();
    run_test_memory_calloc_and_strdup();
    run_test_memory_debug_tracking();

    // CodeGen Tests
    run_test_codegen_context_creation();
    run_test_register_generation();
    run_test_llvm_type_to_string_conversion();
    run_test_codegen_return_variants();
    run_test_codegen_generate_full_program();
    
    // Additional coverage tests
    run_test_print_usage_function();
    run_test_parse_arguments_debug_option();
    run_test_parse_arguments_verbose_option();
    run_test_parse_arguments_dump_ast_option();
    run_test_debug_and_verbose_print_functions();
    run_test_compiler_info_function();
    run_test_create_array_type();
    run_test_duplicate_type_info_with_return_type();
    run_test_ast_conditional_statements();
    run_test_ast_additional_node_types();
    
    std::cout << "\n=====================\n";
    std::cout << "Test Results: " << tests_passed << " passed, " << tests_failed << " failed\n";
    
    return tests_failed;
}

// Simple unit test using Catch2
#include "catch2/catch.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <getopt.h>
#include <unistd.h>

extern "C" {
    #include "../../srccpp/ast.h"
    #include "../../srccpp/error_handling.h"
    #include "../../srccpp/memory_management.h"
    #include "../../srccpp/codegen.h"
    #include "../../srccpp/constants.h"
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
    ASTNode* function_def = create_function_def_node(create_type_info(TYPE_INT), literal(name), NULL, body, 0);
    function_def->next = NULL;
    return function_def;
}

TEST_CASE("AST Node Construction") {
    SECTION("Basic node creation") {
        ASTNode* node = create_ast_node(AST_IDENTIFIER);
        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_IDENTIFIER);
        free_ast_node(node);
    }

    SECTION("Identifier node creation") {
        char name[] = "variable_name";
        ASTNode* node = create_identifier_node(name);
        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_IDENTIFIER);
        REQUIRE(node->data.identifier.name != nullptr);
        REQUIRE(strcmp(node->data.identifier.name, name) == 0);
        free_ast_node(node);
    }

    SECTION("Constant node creation") {
        ASTNode* node = create_constant_node(42, TYPE_INT);
        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_CONSTANT);
        REQUIRE(node->data.constant.const_type == TYPE_INT);
        REQUIRE(node->data.constant.value.int_val == 42);
        free_ast_node(node);
    }

    SECTION("String literal node creation") {
        char input[] = "\"Hello, World!\"";
        char expected[] = "Hello, World!";
        ASTNode* node = create_string_literal_node(input);
        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_STRING_LITERAL);
        REQUIRE(node->data.string_literal.string != nullptr);
        REQUIRE(strcmp(node->data.string_literal.string, expected) == 0);
        REQUIRE(node->data.string_literal.length == (int)strlen(expected));
        free_ast_node(node);
    }

    SECTION("Unary operation node creation") {
        ASTNode* operand = create_constant_node(42, TYPE_INT);
        ASTNode* node = create_unary_op_node(UOP_MINUS, operand);

        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_UNARY_OP);
        REQUIRE(node->data.unary_op.op == UOP_MINUS);
        REQUIRE(node->data.unary_op.operand == operand);

        free_ast_node(node);
    }
}

TEST_CASE("Type and Symbol Management") {
    SECTION("Type info creation") {
        TypeInfo* type = create_type_info(TYPE_INT);
        REQUIRE(type != nullptr);
        REQUIRE(type->base_type == TYPE_INT);
        REQUIRE(type->qualifiers == QUAL_NONE);
        REQUIRE(type->storage_class == STORAGE_NONE);
        REQUIRE(type->pointer_level == 0);
        free_type_info(type);
    }

    SECTION("Array type creation") {
        TypeInfo* base = create_type_info(TYPE_INT);
        TypeInfo* array = create_array_type(base, 10);

        REQUIRE(array != nullptr);
        REQUIRE(array->base_type == TYPE_ARRAY);
        REQUIRE(array->array_size == 10);
        REQUIRE(array->return_type == base);

        free_type_info(array);
    }

    SECTION("Type duplication") {
        TypeInfo* original = create_type_info(TYPE_POINTER);
        original->return_type = create_type_info(TYPE_INT);

        TypeInfo* copy = duplicate_type_info(original);

        REQUIRE(copy != nullptr);
        REQUIRE(copy->base_type == TYPE_POINTER);
        REQUIRE(copy->return_type != nullptr);
        REQUIRE(copy->return_type->base_type == TYPE_INT);

        free_type_info(copy);
        free_type_info(original);
    }

    SECTION("Pointer type creation") {
        TypeInfo* base = create_type_info(TYPE_INT);
        TypeInfo* ptr = create_pointer_type(base);
        REQUIRE(ptr != nullptr);
        REQUIRE(ptr->base_type == TYPE_POINTER);
        REQUIRE(ptr->return_type != nullptr);
        REQUIRE(ptr->return_type->base_type == TYPE_INT);
        REQUIRE(ptr->pointer_level == 1);
        free_type_info(base);
        free_type_info(ptr);
    }

    SECTION("Symbol creation") {
        TypeInfo* type = create_type_info(TYPE_INT);
        char name[] = "test_var";
        Symbol* symbol = create_symbol(name, type);

        REQUIRE(symbol != nullptr);
        REQUIRE(symbol->name != nullptr);
        REQUIRE(strcmp(symbol->name, name) == 0);
        REQUIRE(symbol->type == type);
        REQUIRE(symbol->is_global == false);
        REQUIRE(symbol->is_parameter == false);

        free_symbol(symbol);
    }
}

TEST_CASE("Compiler CLI and Main") {
    SECTION("Parse arguments - flags") {
        reset_compiler_options();
        char prog[] = "ccompiler";
        char flags[] = "-dvta";
        char output_flag[] = "-o";
        char output_file[] = "out.ll";
        char input_file[] = "input.c";
        char* argv[] = {prog, flags, output_flag, output_file, input_file};
        int argc = sizeof(argv) / sizeof(argv[0]);

        int result = parse_arguments(argc, argv);
        REQUIRE(result == 0);
        REQUIRE(options.debug_mode == 1);
        REQUIRE(options.verbose == 1);
        REQUIRE(options.dump_ast == 1);
        REQUIRE(options.dump_tokens == 1);
        REQUIRE(strcmp(options.output_file, output_file) == 0);
        REQUIRE(strcmp(options.input_file, input_file) == 0);
    }

    SECTION("Parse arguments - invalid option") {
        reset_compiler_options();
        char prog[] = "ccompiler";
        char bad_flag[] = "--unknown";
        char* argv[] = {prog, bad_flag};

        int result = parse_arguments(2, argv);
        REQUIRE(result == -1);
    }

    SECTION("ccompiler_main happy path") {
        reset_compiler_options();
        yyin = NULL;

        program_ast = build_stub_function("stub", 1);

        char prog[] = "ccompiler";
        char output_flag[] = "-o";
        char output_file[] = "unit_main.ll";
        char* argv[] = {prog, output_flag, output_file};

        std::remove(output_file);
        int result = ccompiler_main(3, argv);
        REQUIRE(result == 0);

        FILE* produced = fopen(output_file, "r");
        REQUIRE(produced != nullptr);
        char buffer[128];
        REQUIRE(fgets(buffer, sizeof(buffer), produced) != nullptr);
        fclose(produced);
        std::remove(output_file);

        if (program_ast) {
            free_ast_node(program_ast);
            program_ast = NULL;
        }
        reset_compiler_options();
    }

    SECTION("ccompiler_main verbose modes") {
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
        REQUIRE(result == 0);

        FILE* produced = fopen(output_file, "r");
        REQUIRE(produced != nullptr);
        fclose(produced);
        std::remove(output_file);

        if (program_ast) {
            free_ast_node(program_ast);
            program_ast = NULL;
        }
        reset_compiler_options();
    }

    SECTION("Run on fixtures") {
        if (access("./ccompiler", X_OK) == 0) {
            const char* fixtures[] = {
                "arithmetic.c",
                "simple.c",
                "variables.c"
            };

            for (const char* fixture : fixtures) {
                std::string command = std::string("./ccompiler tests/fixtures/") + fixture + " -o unit_fixture.ll > /dev/null 2>&1";
                int rc = system(command.c_str());
                REQUIRE(rc == 0);
                std::remove("unit_fixture.ll");
            }
        }
    }

    SECTION("Main print helpers") {
        reset_compiler_options();
        options.debug_mode = 1;
        options.verbose = 1;

        // Redirect stdout to /dev/null to avoid noisy output during tests
        FILE* original_stdout = stdout;
        FILE* dev_null = fopen("/dev/null", "w");
        if (dev_null) stdout = dev_null;

        print_usage("ccompiler");
        compiler_info();
        debug_print("toggled %d", options.debug_mode);
        verbose_print("output %s", "demo");

        if (dev_null) {
            fclose(dev_null);
            stdout = original_stdout;
        }
    }
}

TEST_CASE("Resource Cleanup") {
    SECTION("Cleanup handles state") {
        program_ast = create_identifier_node(literal("temp"));
        yyin = tmpfile();
        cleanup_resources();
        REQUIRE(program_ast == nullptr);
        REQUIRE(yyin == nullptr);
    }
}

TEST_CASE("Memory Management") {
    SECTION("Context creation") {
        MemoryContext* ctx = create_memory_context();
        REQUIRE(ctx != nullptr);
        REQUIRE(ctx->stats.allocations == 0);
        REQUIRE(ctx->stats.deallocations == 0);
        free_memory_context(ctx);
    }

    SECTION("Tracked allocation") {
        cleanup_memory_management();
        init_memory_management();
        MemoryContext* ctx = g_memory_context;
        REQUIRE(ctx != nullptr);

        void* ptr = safe_malloc_debug(32, "unit", 120, "test");
        REQUIRE(ptr != nullptr);
        REQUIRE(ctx->stats.allocations > 0);

        safe_free_debug(ptr, "unit", 121, "test");
        REQUIRE(ctx->stats.deallocations == ctx->stats.allocations);
        REQUIRE(check_memory_leaks(ctx) == false);

        cleanup_memory_management();
    }
}

TEST_CASE("Code Generation Utilities") {
    SECTION("LLVM type string conversion") {
        TypeInfo* int_type = create_type_info(TYPE_INT);
        TypeInfo* float_type = create_type_info(TYPE_FLOAT);

        char* int_str = llvm_type_to_string(int_type);
        char* float_str = llvm_type_to_string(float_type);
        char* null_str = llvm_type_to_string(nullptr);

        REQUIRE(strcmp(int_str, "i32") == 0);
        REQUIRE(strcmp(float_str, "float") == 0);
        REQUIRE(strcmp(null_str, "void") == 0);

        free(int_str);
        free(float_str);
        free(null_str);
        free_type_info(int_type);
        free_type_info(float_type);
    }

    SECTION("Return statement generation") {
        FILE* output = tmpfile();
        REQUIRE(output != nullptr);
        CodeGenContext* ctx = create_codegen_context(output);
        REQUIRE(ctx != nullptr);

        ASTNode* constant_return = create_return_stmt_node(create_constant_node(7, TYPE_INT));
        generate_return_statement(ctx, constant_return);
        free_ast_node(constant_return);

        ASTNode* void_return = create_ast_node(AST_RETURN_STMT);
        void_return->data.return_stmt.expression = NULL;
        generate_return_statement(ctx, void_return);
        free_ast_node(void_return);

        std::string ir = read_tmp_file(output);
        REQUIRE(ir.find("ret i32 7") != std::string::npos);
        REQUIRE(ir.find("ret void") != std::string::npos);

        free_codegen_context(ctx);
        fclose(output);
    }
}

TEST_CASE("Error Handling") {
    SECTION("Error creation and printing") {
        ErrorContext* error = create_error(ERROR_PARSE, "test.c", 42, "test_function", "Test error message");

        REQUIRE(error != nullptr);
        REQUIRE(error->type == ERROR_PARSE);
        REQUIRE(strcmp(error->file, "test.c") == 0);
        REQUIRE(error->line == 42);
        
        // Just verify it doesn't crash
        print_error(error);
        free_error(error);
    }
}
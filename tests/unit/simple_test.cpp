// Simple unit test without external dependencies
#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>

extern "C" {
    #include "../../src/ast.h"
    #include "../../src/error_handling.h"
    #include "../../src/memory_management.h"
    #include "../../src/codegen.h"
    #include "../../src/constants.h"
}

int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    void test_##name(); \
    void run_test_##name() { \
        std::cout << "Testing " << #name << "... "; \
        try { \
            test_##name(); \
            std::cout << "PASSED
"; \
            tests_passed++; \
        } catch (...) { \
            std::cout << "FAILED
"; \
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
    ASSERT_EQ(ptr->pointer_level, 1);
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
    
    free_error(error);
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

// Test runner
int main() {
    std::cout << "Running Unit Tests...
";
    std::cout << "=====================

";
    
    // AST Tests
    run_test_ast_node_creation();
    run_test_identifier_node_creation();
    run_test_constant_node_creation();
    run_test_string_literal_node_creation();
    run_test_binary_op_node_creation();
    run_test_type_info_creation();
    run_test_pointer_type_creation();
    run_test_symbol_creation();
    
    // Error Handling Tests
    run_test_error_creation();
    
    // Memory Management Tests
    run_test_memory_context_creation();
    run_test_safe_malloc_debug();
    
    // CodeGen Tests
    run_test_codegen_context_creation();
    run_test_register_generation();
    run_test_llvm_type_to_string_conversion();
    
    std::cout << "
=====================
";
    std::cout << "Test Results: " << tests_passed << " passed, " << tests_failed << " failed
";
    
    return tests_failed;
}
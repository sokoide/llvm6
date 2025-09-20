// Basic unit tests for C compiler
#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdio>

extern "C" {
    #include "../../src/ast.h"
    #include "../../src/error_handling.h"
    #include "../../src/memory_management.h"
    #include "../../src/codegen.h"
}

int passed = 0;
int failed = 0;

void test_ast_node_creation() {
    ASTNode* node = create_ast_node(AST_IDENTIFIER);
    if (node && node->type == AST_IDENTIFIER) {
        std::cout << "✓ AST node creation test passed\n";
        passed++;
        free_ast_node(node);
    } else {
        std::cout << "✗ AST node creation test failed\n";
        failed++;
    }
}

void test_identifier_node() {
    char name[] = "test_var";
    ASTNode* node = create_identifier_node(name);
    if (node && node->type == AST_IDENTIFIER && 
        node->data.identifier.name && 
        strcmp(node->data.identifier.name, name) == 0) {
        std::cout << "✓ Identifier node test passed\n";
        passed++;
        free_ast_node(node);
    } else {
        std::cout << "✗ Identifier node test failed\n";
        failed++;
    }
}

void test_constant_node() {
    ASTNode* node = create_constant_node(42, TYPE_INT);
    if (node && node->type == AST_CONSTANT && 
        node->data.constant.const_type == TYPE_INT &&
        node->data.constant.value.int_val == 42) {
        std::cout << "✓ Constant node test passed\n";
        passed++;
        free_ast_node(node);
    } else {
        std::cout << "✗ Constant node test failed\n";
        failed++;
    }
}

void test_string_literal_node() {
    char str[] = "Hello";
    ASTNode* node = create_string_literal_node(str);
    if (node && node->type == AST_STRING_LITERAL &&
        node->data.string_literal.string &&
        strcmp(node->data.string_literal.string, str) == 0 &&
        node->data.string_literal.length == (int)strlen(str)) {
        std::cout << "✓ String literal node test passed\n";
        passed++;
        free_ast_node(node);
    } else {
        std::cout << "✗ String literal node test failed\n";
        failed++;
    }
}

void test_binary_op_node() {
    ASTNode* left = create_constant_node(5, TYPE_INT);
    ASTNode* right = create_constant_node(3, TYPE_INT);
    ASTNode* node = create_binary_op_node(OP_ADD, left, right);
    
    if (node && node->type == AST_BINARY_OP &&
        node->data.binary_op.op == OP_ADD &&
        node->data.binary_op.left == left &&
        node->data.binary_op.right == right) {
        std::cout << "✓ Binary operation node test passed\n";
        passed++;
        free_ast_node(node);
    } else {
        std::cout << "✗ Binary operation node test failed\n";
        failed++;
    }
}

void test_type_info() {
    TypeInfo* type = create_type_info(TYPE_INT);
    if (type && type->base_type == TYPE_INT &&
        type->qualifiers == QUAL_NONE &&
        type->storage_class == STORAGE_NONE &&
        type->pointer_level == 0) {
        std::cout << "✓ Type info test passed\n";
        passed++;
        free_type_info(type);
    } else {
        std::cout << "✗ Type info test failed\n";
        failed++;
    }
}

void test_pointer_type() {
    TypeInfo* base = create_type_info(TYPE_INT);
    TypeInfo* ptr = create_pointer_type(base);
    if (ptr && ptr->base_type == TYPE_POINTER && ptr->pointer_level == 1) {
        std::cout << "✓ Pointer type test passed\n";
        passed++;
        free_type_info(ptr);
    } else {
        std::cout << "✗ Pointer type test failed\n";
        failed++;
    }
}

void test_symbol_creation() {
    TypeInfo* type = create_type_info(TYPE_INT);
    char name[] = "test_symbol";
    Symbol* symbol = create_symbol(name, type);
    
    if (symbol && symbol->name &&
        strcmp(symbol->name, name) == 0 &&
        symbol->type == type &&
        symbol->is_global == false &&
        symbol->is_parameter == false) {
        std::cout << "✓ Symbol creation test passed\n";
        passed++;
        free_symbol(symbol);
    } else {
        std::cout << "✗ Symbol creation test failed\n";
        failed++;
    }
}

void test_error_creation() {
    ErrorContext* error = create_error(ERROR_PARSE, "test.c", 42, "test_function", "Test message");
    
    if (error && error->type == ERROR_PARSE &&
        strcmp(error->file, "test.c") == 0 &&
        error->line == 42 &&
        strcmp(error->function, "test_function") == 0 &&
        strcmp(error->message, "Test message") == 0) {
        std::cout << "✓ Error creation test passed\n";
        passed++;
        free_error(error);
    } else {
        std::cout << "✗ Error creation test failed\n";
        failed++;
    }
}

void test_memory_context() {
    MemoryContext* ctx = create_memory_context();
    if (ctx && ctx->stats.allocations == 0 &&
        ctx->stats.deallocations == 0 &&
        ctx->debug_mode == 0) {
        std::cout << "✓ Memory context test passed\n";
        passed++;
        free_memory_context(ctx);
    } else {
        std::cout << "✗ Memory context test failed\n";
        failed++;
    }
}

void test_safe_malloc() {
    void* ptr = safe_malloc_debug(100, "test.c", 42, "test_function");
    if (ptr) {
        std::cout << "✓ Safe malloc test passed\n";
        passed++;
        safe_free_debug(ptr, "test.c", 43, "test_function");
    } else {
        std::cout << "✗ Safe malloc test failed\n";
        failed++;
    }
}

void test_codegen_context() {
    FILE* output = tmpfile();
    if (output) {
        CodeGenContext* ctx = create_codegen_context(output);
        if (ctx && ctx->next_reg_id == 1 && ctx->next_bb_id == 1) {
            std::cout << "✓ CodeGen context test passed\n";
            passed++;
            free_codegen_context(ctx);
        } else {
            std::cout << "✗ CodeGen context test failed\n";
            failed++;
        }
        fclose(output);
    } else {
        std::cout << "✗ CodeGen context test failed (tmpfile)\n";
        failed++;
    }
}

void test_register_generation() {
    FILE* output = tmpfile();
    if (output) {
        CodeGenContext* ctx = create_codegen_context(output);
        char* reg1 = get_next_register(ctx);
        char* reg2 = get_next_register(ctx);
        
        if (reg1 && reg2 && 
            strcmp(reg1, "1") == 0 && 
            strcmp(reg2, "2") == 0) {
            std::cout << "✓ Register generation test passed\n";
            passed++;
        } else {
            std::cout << "✗ Register generation test failed\n";
            failed++;
        }
        
        if (reg1) free(reg1);
        if (reg2) free(reg2);
        free_codegen_context(ctx);
        fclose(output);
    } else {
        std::cout << "✗ Register generation test failed (tmpfile)\n";
        failed++;
    }
}

void test_llvm_type_conversion() {
    TypeInfo* int_type = create_type_info(TYPE_INT);
    TypeInfo* float_type = create_type_info(TYPE_FLOAT);
    
    char* int_str = llvm_type_to_string(int_type);
    char* float_str = llvm_type_to_string(float_type);
    char* null_str = llvm_type_to_string(nullptr);
    
    if (int_str && float_str && null_str &&
        strcmp(int_str, "i32") == 0 &&
        strcmp(float_str, "float") == 0 &&
        strcmp(null_str, "void") == 0) {
        std::cout << "✓ LLVM type conversion test passed\n";
        passed++;
    } else {
        std::cout << "✗ LLVM type conversion test failed\n";
        failed++;
    }
    
    if (int_str) free(int_str);
    if (float_str) free(float_str);
    if (null_str) free(null_str);
    free_type_info(int_type);
    free_type_info(float_type);
}

int main() {
    std::cout << "Running Unit Tests...\n";
    std::cout << "===================\n";
    
    // Run all tests
    test_ast_node_creation();
    test_identifier_node();
    test_constant_node();
    test_string_literal_node();
    test_binary_op_node();
    test_type_info();
    test_pointer_type();
    test_symbol_creation();
    test_error_creation();
    test_memory_context();
    test_safe_malloc();
    test_codegen_context();
    test_register_generation();
    test_llvm_type_conversion();
    
    std::cout << "===================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    
    if (failed == 0) {
        std::cout << "🎉 All tests passed!\n";
    } else {
        std::cout << "⚠️  " << failed << " test(s) failed\n";
    }
    
    return failed;
}
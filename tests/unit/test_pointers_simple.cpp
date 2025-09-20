#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdio>

extern "C" {
    #include "../../src/ast.h"
    #include "../../src/codegen.h"
    #include "../../src/memory_management.h"
    #include "../../src/constants.h"
}

int pointer_tests_passed = 0;
int pointer_tests_failed = 0;

void test_pointer_declaration() {
    std::cout << "Testing pointer type declaration...\n";
    
    TypeInfo* int_type = create_type_info(TYPE_INT);
    TypeInfo* ptr_type = create_pointer_type(int_type);
    
    if (ptr_type && ptr_type->base_type == TYPE_POINTER && ptr_type->pointer_level == 1) {
        std::cout << "✓ Pointer declaration test passed\n";
        pointer_tests_passed++;
    } else {
        std::cout << "✗ Pointer declaration test failed - pointer_level or base_type incorrect\n";
        pointer_tests_failed++;
    }
    
    free_type_info(ptr_type);
    free_type_info(int_type);
}

void test_address_of_operator() {
    std::cout << "Testing address-of operator (&x)...\n";
    
    FILE* output = tmpfile();
    if (!output) {
        output = stdout;
    }
    CodeGenContext* ctx = create_codegen_context(output);
    
    // Create variable symbol: int x;
    Symbol* var_symbol = create_symbol(safe_strdup("x"), create_type_info(TYPE_INT));
    add_local_symbol(ctx, var_symbol);
    
    // Create address-of expression: &x
    ASTNode* addr_expr = create_ast_node(AST_UNARY_OP);
    addr_expr->data.unary_op.op = UOP_ADDR;
    addr_expr->data.unary_op.operand = create_identifier_node(safe_strdup("x"));
    
    // This should generate proper LLVM IR for address-of (expected to fail initially)
    LLVMValue* result = generate_unary_op(ctx, addr_expr);
    
    if (result && result->type == LLVM_VALUE_REGISTER && result->llvm_type && result->llvm_type->base_type == TYPE_POINTER) {
        std::cout << "✓ Address-of operator test passed\n";
        pointer_tests_passed++;
    } else {
        std::cout << "✗ Address-of operator test failed - incomplete pointer support (EXPECTED)\n";
        pointer_tests_failed++;
    }
    
    free_ast_node(addr_expr);
    if (result) free_llvm_value(result);
    free_codegen_context(ctx);
    if (output && output != stdout) {
        fclose(output);
    }
}

void test_dereference_operator() {
    std::cout << "Testing dereference operator (*ptr)...\n";
    
    FILE* output = tmpfile();
    if (!output) {
        output = stdout;
    }
    CodeGenContext* ctx = create_codegen_context(output);
    
    // Create pointer symbol: int* ptr;
    TypeInfo* ptr_type = create_pointer_type(create_type_info(TYPE_INT));
    Symbol* ptr_symbol = create_symbol(safe_strdup("ptr"), ptr_type);
    add_local_symbol(ctx, ptr_symbol);
    
    // Create dereference expression: *ptr
    ASTNode* deref_expr = create_ast_node(AST_UNARY_OP);
    deref_expr->data.unary_op.op = UOP_DEREF;
    deref_expr->data.unary_op.operand = create_identifier_node(safe_strdup("ptr"));
    
    // This should generate proper LLVM IR for dereference (expected to fail initially)
    LLVMValue* result = generate_unary_op(ctx, deref_expr);
    
    if (result && result->type == LLVM_VALUE_REGISTER && result->llvm_type && result->llvm_type->base_type == TYPE_INT) {
        std::cout << "✓ Dereference operator test passed\n";
        pointer_tests_passed++;
    } else {
        std::cout << "✗ Dereference operator test failed - incomplete pointer support (EXPECTED)\n";
        pointer_tests_failed++;
    }
    
    free_ast_node(deref_expr);
    if (result) free_llvm_value(result);
    free_codegen_context(ctx);
    if (output && output != stdout) {
        fclose(output);
    }
}

void test_pointer_arithmetic() {
    std::cout << "Testing pointer arithmetic (ptr + 1)...\n";
    
    FILE* output = tmpfile();
    if (!output) {
        output = stdout;
    }
    CodeGenContext* ctx = create_codegen_context(output);
    
    // Create pointer symbol: int* ptr;
    TypeInfo* ptr_type = create_pointer_type(create_type_info(TYPE_INT));
    Symbol* ptr_symbol = create_symbol(safe_strdup("ptr"), ptr_type);
    add_local_symbol(ctx, ptr_symbol);
    
    // Create expression: ptr + 1
    ASTNode* add_expr = create_ast_node(AST_BINARY_OP);
    add_expr->data.binary_op.op = OP_ADD;
    add_expr->data.binary_op.left = create_identifier_node(safe_strdup("ptr"));
    add_expr->data.binary_op.right = create_constant_node(1, TYPE_INT);
    
    // This should generate proper pointer arithmetic (expected to fail initially)
    LLVMValue* result = generate_binary_op(ctx, add_expr);
    
    if (result && result->type == LLVM_VALUE_REGISTER && result->llvm_type && result->llvm_type->base_type == TYPE_POINTER) {
        std::cout << "✓ Pointer arithmetic test passed\n";
        pointer_tests_passed++;
    } else {
        std::cout << "✗ Pointer arithmetic test failed - incomplete pointer arithmetic (EXPECTED)\n";
        pointer_tests_failed++;
    }
    
    free_ast_node(add_expr);
    if (result) free_llvm_value(result);
    free_codegen_context(ctx);
    if (output && output != stdout) {
        fclose(output);
    }
}

void test_multi_level_pointers() {
    std::cout << "Testing multi-level pointers (int**)...\n";
    
    TypeInfo* int_type = create_type_info(TYPE_INT);
    TypeInfo* ptr_type = create_pointer_type(int_type);
    TypeInfo* ptr_ptr_type = create_pointer_type(ptr_type);
    
    if (ptr_ptr_type && ptr_ptr_type->base_type == TYPE_POINTER && ptr_ptr_type->pointer_level == 2) {
        std::cout << "✓ Multi-level pointers test passed\n";
        pointer_tests_passed++;
    } else {
        std::cout << "✗ Multi-level pointers test failed - incorrect pointer_level (EXPECTED)\n";
        pointer_tests_failed++;
    }
    
    free_type_info(ptr_ptr_type);
    free_type_info(ptr_type);
    free_type_info(int_type);
}

void run_pointer_tests() {
    std::cout << "\n=== Running Pointer Feature Tests (Expected to Fail) ===\n";
    
    test_pointer_declaration();
    test_address_of_operator(); 
    test_dereference_operator();
    test_pointer_arithmetic();
    test_multi_level_pointers();
    
    std::cout << "\n=== Pointer Test Results ===\n";
    std::cout << "Passed: " << pointer_tests_passed << "\n";
    std::cout << "Failed: " << pointer_tests_failed << " (Expected failures due to incomplete implementation)\n";
    std::cout << "Total:  " << (pointer_tests_passed + pointer_tests_failed) << "\n";
}
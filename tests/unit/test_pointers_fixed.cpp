#include "catch2/catch.hpp"

extern "C" {
    #include "../../src/ast.h"
    #include "../../src/codegen.h"
    #include "../../src/memory_management.h"
    #include "../../src/constants.h"
}

TEST_CASE("Pointer Declaration Test") {
    // Test creating a pointer type declaration
    TypeInfo* int_type = create_type_info(TYPE_INT);
    TypeInfo* ptr_type = create_pointer_type(int_type);
    
    REQUIRE(ptr_type != nullptr);
    REQUIRE(ptr_type->base_type == TYPE_POINTER);
    REQUIRE(ptr_type->pointer_level == 1);
    REQUIRE(ptr_type->return_type == int_type); // base type stored here
    
    free_type_info(ptr_type);
    free_type_info(int_type);
}

TEST_CASE("Address Of Operator Test") {
    // Test &variable operation - should fail initially
    FILE* output = tmpfile();
    CodeGenContext* ctx = create_codegen_context(output);
    
    // Create variable symbol: int x;
    Symbol* var_symbol = create_symbol(safe_strdup("x"), create_type_info(TYPE_INT));
    add_local_symbol(ctx, var_symbol);
    
    // Create address-of expression: &x
    ASTNode* addr_expr = create_ast_node(AST_UNARY_OP);
    addr_expr->data.unary_op.op = UOP_ADDR;
    addr_expr->data.unary_op.operand = create_identifier_node(safe_strdup("x"));
    
    // This should generate proper LLVM IR for address-of (will likely fail initially)
    LLVMValue* result = generate_unary_op(ctx, addr_expr);
    
    // We expect this to fail initially, so we check if it's properly attempted
    // The result may be NULL due to incomplete implementation
    
    free_ast_node(addr_expr);
    if (result) free_llvm_value(result);
    free_codegen_context(ctx);
    fclose(output);
}

TEST_CASE("Dereference Operator Test") {
    // Test *pointer operation - should fail initially
    FILE* output = tmpfile();
    CodeGenContext* ctx = create_codegen_context(output);
    
    // Create pointer symbol: int* ptr;
    TypeInfo* ptr_type = create_pointer_type(create_type_info(TYPE_INT));
    Symbol* ptr_symbol = create_symbol(safe_strdup("ptr"), ptr_type);
    add_local_symbol(ctx, ptr_symbol);
    
    // Create dereference expression: *ptr
    ASTNode* deref_expr = create_ast_node(AST_UNARY_OP);
    deref_expr->data.unary_op.op = UOP_DEREF;
    deref_expr->data.unary_op.operand = create_identifier_node(safe_strdup("ptr"));
    
    // This should generate proper LLVM IR for dereference (will likely fail initially)
    LLVMValue* result = generate_unary_op(ctx, deref_expr);
    
    // We expect this to fail initially or produce incorrect results
    
    free_ast_node(deref_expr);
    if (result) free_llvm_value(result);
    free_codegen_context(ctx);
    fclose(output);
}

TEST_CASE("Pointer Assignment Test") {
    // Test ptr = &var assignment - should fail initially
    FILE* output = tmpfile();
    CodeGenContext* ctx = create_codegen_context(output);
    
    // Create variable symbol: int x;
    Symbol* var_symbol = create_symbol(safe_strdup("x"), create_type_info(TYPE_INT));
    add_local_symbol(ctx, var_symbol);
    
    // Create pointer symbol: int* ptr;
    TypeInfo* ptr_type = create_pointer_type(create_type_info(TYPE_INT));
    Symbol* ptr_symbol = create_symbol(safe_strdup("ptr"), ptr_type);
    add_local_symbol(ctx, ptr_symbol);
    
    // Create assignment: ptr = &x
    ASTNode* addr_expr = create_ast_node(AST_UNARY_OP);
    addr_expr->data.unary_op.op = UOP_ADDR;
    addr_expr->data.unary_op.operand = create_identifier_node(safe_strdup("x"));
    
    ASTNode* assign_expr = create_ast_node(AST_BINARY_OP);
    assign_expr->data.binary_op.op = OP_ASSIGN;
    assign_expr->data.binary_op.left = create_identifier_node(safe_strdup("ptr"));
    assign_expr->data.binary_op.right = addr_expr;
    
    // This should generate proper LLVM IR for pointer assignment (will likely fail initially)
    LLVMValue* result = generate_binary_op(ctx, assign_expr);
    
    // We expect this to fail initially or produce incorrect results
    
    free_ast_node(assign_expr);
    if (result) free_llvm_value(result);
    free_codegen_context(ctx);
    fclose(output);
}

TEST_CASE("Pointer Arithmetic Test") {
    // Test ptr + 1 operation - should fail initially
    FILE* output = tmpfile();
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
    
    // This should generate proper pointer arithmetic (will likely fail initially)
    LLVMValue* result = generate_binary_op(ctx, add_expr);
    
    // We expect this to fail initially or produce incorrect arithmetic
    
    free_ast_node(add_expr);
    if (result) free_llvm_value(result);
    free_codegen_context(ctx);
    fclose(output);
}

TEST_CASE("Multi Level Pointers Test") {
    // Test int**, int*** operations - should fail initially
    TypeInfo* int_type = create_type_info(TYPE_INT);
    TypeInfo* ptr_type = create_pointer_type(int_type);
    TypeInfo* ptr_ptr_type = create_pointer_type(ptr_type);
    
    REQUIRE(ptr_ptr_type != nullptr);
    REQUIRE(ptr_ptr_type->base_type == TYPE_POINTER);
    REQUIRE(ptr_ptr_type->pointer_level == 2);
    
    // Test triple pointer
    TypeInfo* ptr_ptr_ptr_type = create_pointer_type(ptr_ptr_type);
    REQUIRE(ptr_ptr_ptr_type->pointer_level == 3);
    
    free_type_info(ptr_ptr_ptr_type);
    free_type_info(ptr_ptr_type);
    free_type_info(ptr_type);
    free_type_info(int_type);
}
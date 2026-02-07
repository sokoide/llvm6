#include "catch2/catch.hpp"
#include <iostream>
#include <cstring>
#include <cstdio>

extern "C" {
    #include "../../src/ast.h"
    #include "../../src/codegen.h"
    #include "../../src/memory_management.h"
    #include "../../src/constants.h"
}

TEST_CASE("Pointer Features") {
    SECTION("Pointer type declaration") {
        TypeInfo* int_type = create_type_info(TYPE_INT);
        TypeInfo* ptr_type = create_pointer_type(int_type);
        
        REQUIRE(ptr_type != nullptr);
        REQUIRE(ptr_type->base_type == TYPE_POINTER);
        REQUIRE(ptr_type->pointer_level == 1);
        
        free_type_info(ptr_type);
        free_type_info(int_type);
    }

    SECTION("Address-of operator (&x)") {
        FILE* output = tmpfile();
        REQUIRE(output != nullptr);
        CodeGenContext* ctx = create_codegen_context(output);
        
        // Create variable symbol: int x;
        Symbol* var_symbol = create_symbol(safe_strdup("x"), create_type_info(TYPE_INT));
        add_local_symbol(ctx, var_symbol);
        
        // Create address-of expression: &x
        ASTNode* addr_expr = create_ast_node(AST_UNARY_OP);
        addr_expr->data.unary_op.op = UOP_ADDR;
        addr_expr->data.unary_op.operand = create_identifier_node(safe_strdup("x"));
        
        LLVMValue* result = generate_unary_op(ctx, addr_expr);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        REQUIRE(result->llvm_type != nullptr);
        REQUIRE(result->llvm_type->base_type == TYPE_POINTER);
        
        free_ast_node(addr_expr);
        if (result) free_llvm_value(result);
        free_codegen_context(ctx);
        fclose(output);
    }

    SECTION("Dereference operator (*ptr)") {
        FILE* output = tmpfile();
        REQUIRE(output != nullptr);
        CodeGenContext* ctx = create_codegen_context(output);
        
        // Create pointer symbol: int* ptr;
        TypeInfo* ptr_type = create_pointer_type(create_type_info(TYPE_INT));
        Symbol* ptr_symbol = create_symbol(safe_strdup("ptr"), ptr_type);
        add_local_symbol(ctx, ptr_symbol);
        
        // Create dereference expression: *ptr
        ASTNode* deref_expr = create_ast_node(AST_UNARY_OP);
        deref_expr->data.unary_op.op = UOP_DEREF;
        deref_expr->data.unary_op.operand = create_identifier_node(safe_strdup("ptr"));
        
        LLVMValue* result = generate_unary_op(ctx, deref_expr);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        REQUIRE(result->llvm_type != nullptr);
        REQUIRE(result->llvm_type->base_type == TYPE_INT);
        
        free_ast_node(deref_expr);
        if (result) free_llvm_value(result);
        free_codegen_context(ctx);
        fclose(output);
    }

    SECTION("Pointer arithmetic (ptr + 1)") {
        FILE* output = tmpfile();
        REQUIRE(output != nullptr);
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
        
        LLVMValue* result = generate_binary_op(ctx, add_expr);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        REQUIRE(result->llvm_type != nullptr);
        REQUIRE(result->llvm_type->base_type == TYPE_POINTER);
        
        free_ast_node(add_expr);
        if (result) free_llvm_value(result);
        free_codegen_context(ctx);
        fclose(output);
    }

    SECTION("Multi-level pointers (int**)") {
        TypeInfo* int_type = create_type_info(TYPE_INT);
        TypeInfo* ptr_type = create_pointer_type(int_type);
        TypeInfo* ptr_ptr_type = create_pointer_type(ptr_type);
        
        REQUIRE(ptr_ptr_type != nullptr);
        REQUIRE(ptr_ptr_type->base_type == TYPE_POINTER);
        REQUIRE(ptr_ptr_type->pointer_level == 2);
        
        free_type_info(ptr_ptr_type);
        free_type_info(ptr_type);
        free_type_info(int_type);
    }
}

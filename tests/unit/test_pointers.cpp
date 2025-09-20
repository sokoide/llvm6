#include "catch2/catch.hpp"

extern "C" {
    #include "../../src/ast.h"
    #include "../../src/codegen.h"
    #include "../../src/memory_management.h"
    #include "../../src/constants.h"
}

TEST_CASE("Basic Pointer Operations") {
    SECTION("test_pointer_declaration - should fail initially") {
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
    
    SECTION("test_address_of_operator - should fail initially") {
        // Test &variable operation
        CodeGenContext* ctx = create_codegen_context();
        
        // Create variable: int x = 42;
        ASTNode* var_decl = create_ast_node(AST_VARIABLE_DECL);
        var_decl->data.variable_decl.name = safe_strdup("x");
        var_decl->data.variable_decl.type = create_type_info(TYPE_INT);
        var_decl->data.variable_decl.initializer = create_constant_node(42);
        
        // Create address-of expression: &x
        ASTNode* addr_expr = create_ast_node(AST_UNARY_OP);
        addr_expr->data.unary_op.op = UOP_ADDR;
        addr_expr->data.unary_op.operand = create_identifier_node("x");
        
        // This should generate proper LLVM IR for address-of
        LLVMValue* result = generate_unary_op(ctx, addr_expr);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        REQUIRE(result->llvm_type != nullptr);
        REQUIRE(result->llvm_type->base_type == TYPE_POINTER);
        
        free_ast_node(var_decl);
        free_ast_node(addr_expr);
        free_llvm_value(result);
        free_codegen_context(ctx);
    }
    
    SECTION("test_dereference_operator - should fail initially") {
        // Test *pointer operation
        CodeGenContext* ctx = create_codegen_context();
        
        // Create pointer variable: int* ptr;
        Symbol* ptr_symbol = create_symbol();
        ptr_symbol->name = safe_strdup("ptr");
        ptr_symbol->type = create_pointer_type(create_type_info(TYPE_INT));
        add_local_symbol(ctx, ptr_symbol);
        
        // Create dereference expression: *ptr
        ASTNode* deref_expr = create_ast_node(AST_UNARY_OP);
        deref_expr->data.unary_op.op = UOP_DEREF;
        deref_expr->data.unary_op.operand = create_identifier_node("ptr");
        
        // This should generate proper LLVM IR for dereference
        LLVMValue* result = generate_unary_op(ctx, deref_expr);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        REQUIRE(result->llvm_type != nullptr);
        REQUIRE(result->llvm_type->base_type == TYPE_INT);
        
        free_ast_node(deref_expr);
        free_llvm_value(result);
        free_codegen_context(ctx);
    }
    
    SECTION("test_pointer_assignment - should fail initially") {
        // Test ptr = &var assignment
        CodeGenContext* ctx = create_codegen_context();
        
        // Create variable: int x = 42;
        Symbol* var_symbol = create_symbol();
        var_symbol->name = safe_strdup("x");
        var_symbol->type = create_type_info(TYPE_INT);
        add_local_symbol(ctx, var_symbol);
        
        // Create pointer: int* ptr;
        Symbol* ptr_symbol = create_symbol();
        ptr_symbol->name = safe_strdup("ptr");
        ptr_symbol->type = create_pointer_type(create_type_info(TYPE_INT));
        add_local_symbol(ctx, ptr_symbol);
        
        // Create assignment: ptr = &x
        ASTNode* addr_expr = create_ast_node(AST_UNARY_OP);
        addr_expr->data.unary_op.op = UOP_ADDR;
        addr_expr->data.unary_op.operand = create_identifier_node("x");
        
        ASTNode* assign_expr = create_ast_node(AST_BINARY_OP);
        assign_expr->data.binary_op.op = BOP_ASSIGN;
        assign_expr->data.binary_op.left = create_identifier_node("ptr");
        assign_expr->data.binary_op.right = addr_expr;
        
        // This should generate proper LLVM IR for pointer assignment
        LLVMValue* result = generate_binary_op(ctx, assign_expr);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        
        free_ast_node(assign_expr);
        free_llvm_value(result);
        free_codegen_context(ctx);
    }
    
    SECTION("test_null_pointer - should fail initially") {
        // Test NULL pointer handling
        TypeInfo* ptr_type = create_pointer_type(create_type_info(TYPE_INT));
        
        // Create NULL constant
        ASTNode* null_node = create_ast_node(AST_CONSTANT);
        null_node->data.constant.type = CONST_INT;
        null_node->data.constant.int_value = 0; // NULL is typically 0
        
        // This should be compatible with pointer types
        bool is_compatible = types_compatible(ptr_type, null_node->data.constant.type);
        REQUIRE(is_compatible == true);
        
        free_type_info(ptr_type);
        free_ast_node(null_node);
    }
}

TEST_CASE("Pointer Arithmetic Operations") {
    SECTION("test_pointer_integer_arithmetic - should fail initially") {
        // Test ptr + 1, ptr - 1 operations
        CodeGenContext* ctx = create_codegen_context();
        
        // Create pointer: int* ptr;
        Symbol* ptr_symbol = create_symbol();
        ptr_symbol->name = safe_strdup("ptr");
        ptr_symbol->type = create_pointer_type(create_type_info(TYPE_INT));
        add_local_symbol(ctx, ptr_symbol);
        
        // Create expression: ptr + 1
        ASTNode* add_expr = create_ast_node(AST_BINARY_OP);
        add_expr->data.binary_op.op = BOP_ADD;
        add_expr->data.binary_op.left = create_identifier_node("ptr");
        add_expr->data.binary_op.right = create_constant_node(1);
        
        // This should generate proper pointer arithmetic
        LLVMValue* result = generate_binary_op(ctx, add_expr);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        REQUIRE(result->llvm_type->base_type == TYPE_POINTER);
        
        free_ast_node(add_expr);
        free_llvm_value(result);
        free_codegen_context(ctx);
    }
    
    SECTION("test_pointer_difference - should fail initially") {
        // Test ptr1 - ptr2 operation
        CodeGenContext* ctx = create_codegen_context();
        
        // Create two pointers: int* ptr1, ptr2;
        Symbol* ptr1_symbol = create_symbol();
        ptr1_symbol->name = safe_strdup("ptr1");
        ptr1_symbol->type = create_pointer_type(create_type_info(TYPE_INT));
        add_local_symbol(ctx, ptr1_symbol);
        
        Symbol* ptr2_symbol = create_symbol();
        ptr2_symbol->name = safe_strdup("ptr2");
        ptr2_symbol->type = create_pointer_type(create_type_info(TYPE_INT));
        add_local_symbol(ctx, ptr2_symbol);
        
        // Create expression: ptr1 - ptr2
        ASTNode* sub_expr = create_ast_node(AST_BINARY_OP);
        sub_expr->data.binary_op.op = BOP_SUB;
        sub_expr->data.binary_op.left = create_identifier_node("ptr1");
        sub_expr->data.binary_op.right = create_identifier_node("ptr2");
        
        // This should generate proper pointer difference (result is integer)
        LLVMValue* result = generate_binary_op(ctx, sub_expr);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        REQUIRE(result->llvm_type->base_type == TYPE_INT); // difference is integer
        
        free_ast_node(sub_expr);
        free_llvm_value(result);
        free_codegen_context(ctx);
    }
    
    SECTION("test_pointer_comparison - should fail initially") {
        // Test ptr1 == ptr2, ptr1 < ptr2 operations
        CodeGenContext* ctx = create_codegen_context();
        
        // Create two pointers
        Symbol* ptr1_symbol = create_symbol();
        ptr1_symbol->name = safe_strdup("ptr1");
        ptr1_symbol->type = create_pointer_type(create_type_info(TYPE_INT));
        add_local_symbol(ctx, ptr1_symbol);
        
        Symbol* ptr2_symbol = create_symbol();
        ptr2_symbol->name = safe_strdup("ptr2");
        ptr2_symbol->type = create_pointer_type(create_type_info(TYPE_INT));
        add_local_symbol(ctx, ptr2_symbol);
        
        // Test equality: ptr1 == ptr2
        ASTNode* eq_expr = create_ast_node(AST_BINARY_OP);
        eq_expr->data.binary_op.op = BOP_EQ;
        eq_expr->data.binary_op.left = create_identifier_node("ptr1");
        eq_expr->data.binary_op.right = create_identifier_node("ptr2");
        
        LLVMValue* eq_result = generate_binary_op(ctx, eq_expr);
        
        REQUIRE(eq_result != nullptr);
        REQUIRE(eq_result->type == LLVM_VALUE_REGISTER);
        REQUIRE(eq_result->llvm_type->base_type == TYPE_INT); // boolean result
        
        free_ast_node(eq_expr);
        free_llvm_value(eq_result);
        free_codegen_context(ctx);
    }
}

TEST_CASE("Advanced Pointer Features") {
    SECTION("test_multi_level_pointers - should fail initially") {
        // Test int**, int*** operations
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
    
    SECTION("test_pointer_to_pointer_assignment - should fail initially") {
        // Test **ptr operations
        CodeGenContext* ctx = create_codegen_context();
        
        // Create double pointer: int** ptr;
        Symbol* ptr_symbol = create_symbol();
        ptr_symbol->name = safe_strdup("ptr");
        TypeInfo* int_type = create_type_info(TYPE_INT);
        TypeInfo* ptr_type = create_pointer_type(int_type);
        ptr_symbol->type = create_pointer_type(ptr_type);
        add_local_symbol(ctx, ptr_symbol);
        
        // Create double dereference: **ptr
        ASTNode* deref1 = create_ast_node(AST_UNARY_OP);
        deref1->data.unary_op.op = UOP_DEREF;
        deref1->data.unary_op.operand = create_identifier_node("ptr");
        
        ASTNode* deref2 = create_ast_node(AST_UNARY_OP);
        deref2->data.unary_op.op = UOP_DEREF;
        deref2->data.unary_op.operand = deref1;
        
        LLVMValue* result = generate_unary_op(ctx, deref2);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->llvm_type->base_type == TYPE_INT);
        
        free_ast_node(deref2);
        free_llvm_value(result);
        free_codegen_context(ctx);
    }
    
    SECTION("test_array_pointer_decay - should fail initially") {
        // Test array[i] vs *(array + i) equivalence
        CodeGenContext* ctx = create_codegen_context();
        
        // Create array: int arr[10];
        Symbol* arr_symbol = create_symbol();
        arr_symbol->name = safe_strdup("arr");
        arr_symbol->type = create_array_type(create_type_info(TYPE_INT), 10);
        add_local_symbol(ctx, arr_symbol);
        
        // Create expression: *(arr + 2)
        ASTNode* add_expr = create_ast_node(AST_BINARY_OP);
        add_expr->data.binary_op.op = BOP_ADD;
        add_expr->data.binary_op.left = create_identifier_node("arr");
        add_expr->data.binary_op.right = create_constant_node(2);
        
        ASTNode* deref_expr = create_ast_node(AST_UNARY_OP);
        deref_expr->data.unary_op.op = UOP_DEREF;
        deref_expr->data.unary_op.operand = add_expr;
        
        LLVMValue* result = generate_unary_op(ctx, deref_expr);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->llvm_type->base_type == TYPE_INT);
        
        free_ast_node(deref_expr);
        free_llvm_value(result);
        free_codegen_context(ctx);
    }
    
    SECTION("test_pointer_function_parameters - should fail initially") {
        // Test void func(int* ptr) parameter passing
        TypeInfo* int_type = create_type_info(TYPE_INT);
        TypeInfo* ptr_type = create_pointer_type(int_type);
        
        // Create function parameter
        ASTNode* param = create_ast_node(AST_PARAMETER);
        param->data.parameter.name = safe_strdup("ptr");
        param->data.parameter.type = ptr_type;
        
        // Create function declaration
        ASTNode* func_decl = create_ast_node(AST_FUNCTION_DECL);
        func_decl->data.function_decl.name = safe_strdup("test_func");
        func_decl->data.function_decl.return_type = create_type_info(TYPE_VOID);
        func_decl->data.function_decl.parameters = param;
        
        REQUIRE(func_decl != nullptr);
        REQUIRE(func_decl->data.function_decl.parameters != nullptr);
        REQUIRE(func_decl->data.function_decl.parameters->data.parameter.type->base_type == TYPE_POINTER);
        
        free_ast_node(func_decl);
    }
}
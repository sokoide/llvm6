#include "catch2/catch.hpp"

extern "C" {
    #include "../../srccpp/ast.h"
    #include "../../srccpp/codegen.h"
    #include "../../srccpp/memory_management.h"
    #include "../../srccpp/constants.h"
}

TEST_CASE("Basic Struct Operations") {
    SECTION("test_struct_declaration - should fail initially") {
        // Test struct definition: struct Point { int x; int y; };
        ASTNode* struct_decl = create_ast_node(AST_STRUCT_DECL);
        struct_decl->data.struct_decl.name = safe_strdup("Point");
        
        // Create member x
        ASTNode* member_x = create_ast_node(AST_VARIABLE_DECL);
        member_x->data.variable_decl.name = safe_strdup("x");
        member_x->data.variable_decl.type = create_type_info(TYPE_INT);
        
        // Create member y  
        ASTNode* member_y = create_ast_node(AST_VARIABLE_DECL);
        member_y->data.variable_decl.name = safe_strdup("y");
        member_y->data.variable_decl.type = create_type_info(TYPE_INT);
        
        // Link members
        member_x->next = member_y;
        struct_decl->data.struct_decl.members = member_x;
        
        REQUIRE(struct_decl != nullptr);
        REQUIRE(strcmp(struct_decl->data.struct_decl.name, "Point") == 0);
        REQUIRE(struct_decl->data.struct_decl.members != nullptr);
        REQUIRE(struct_decl->data.struct_decl.members->next != nullptr);
        
        free_ast_node(struct_decl);
    }
    
    SECTION("test_struct_member_access - should fail initially") {
        // Test obj.member access
        CodeGenContext* ctx = create_codegen_context();
        
        // Create struct type
        TypeInfo* struct_type = create_type_info(TYPE_STRUCT);
        struct_type->struct_name = safe_strdup("Point");
        
        // Create struct variable: struct Point p;
        Symbol* struct_symbol = create_symbol();
        struct_symbol->name = safe_strdup("p");
        struct_symbol->type = struct_type;
        add_local_symbol(ctx, struct_symbol);
        
        // Create member access: p.x
        ASTNode* member_access = create_ast_node(AST_MEMBER_ACCESS);
        member_access->data.member_access.object = create_identifier_node("p");
        member_access->data.member_access.member = safe_strdup("x");
        member_access->data.member_access.is_pointer_access = 0; // dot operator
        
        LLVMValue* result = generate_member_access(ctx, member_access);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        REQUIRE(result->llvm_type->base_type == TYPE_INT);
        
        free_ast_node(member_access);
        free_llvm_value(result);
        free_codegen_context(ctx);
    }
    
    SECTION("test_struct_pointer_access - should fail initially") {
        // Test obj->member access
        CodeGenContext* ctx = create_codegen_context();
        
        // Create struct type
        TypeInfo* struct_type = create_type_info(TYPE_STRUCT);
        struct_type->struct_name = safe_strdup("Point");
        
        // Create pointer to struct: struct Point* ptr;
        Symbol* ptr_symbol = create_symbol();
        ptr_symbol->name = safe_strdup("ptr");
        ptr_symbol->type = create_pointer_type(struct_type);
        add_local_symbol(ctx, ptr_symbol);
        
        // Create pointer member access: ptr->x
        ASTNode* member_access = create_ast_node(AST_MEMBER_ACCESS);
        member_access->data.member_access.object = create_identifier_node("ptr");
        member_access->data.member_access.member = safe_strdup("x");
        member_access->data.member_access.is_pointer_access = 1; // arrow operator
        
        LLVMValue* result = generate_member_access(ctx, member_access);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        REQUIRE(result->llvm_type->base_type == TYPE_INT);
        
        free_ast_node(member_access);
        free_llvm_value(result);
        free_codegen_context(ctx);
    }
    
    SECTION("test_struct_initialization - should fail initially") {
        // Test struct Point p = {10, 20};
        CodeGenContext* ctx = create_codegen_context();
        
        // Create struct type
        TypeInfo* struct_type = create_type_info(TYPE_STRUCT);
        struct_type->struct_name = safe_strdup("Point");
        
        // Create initializer list
        ASTNode* init_x = create_constant_node(10);
        ASTNode* init_y = create_constant_node(20);
        init_x->next = init_y;
        
        ASTNode* initializer = create_ast_node(AST_INITIALIZER_LIST);
        initializer->data.initializer_list.expressions = init_x;
        
        // Create struct variable with initialization
        ASTNode* struct_var = create_ast_node(AST_VARIABLE_DECL);
        struct_var->data.variable_decl.name = safe_strdup("p");
        struct_var->data.variable_decl.type = struct_type;
        struct_var->data.variable_decl.initializer = initializer;
        
        REQUIRE(struct_var != nullptr);
        REQUIRE(struct_var->data.variable_decl.initializer != nullptr);
        REQUIRE(struct_var->data.variable_decl.initializer->type == AST_INITIALIZER_LIST);
        
        free_ast_node(struct_var);
        free_codegen_context(ctx);
    }
}

TEST_CASE("Advanced Struct Features") {
    SECTION("test_nested_structs - should fail initially") {
        // Test struct Rectangle { struct Point top_left; struct Point bottom_right; };
        ASTNode* struct_decl = create_ast_node(AST_STRUCT_DECL);
        struct_decl->data.struct_decl.name = safe_strdup("Rectangle");
        
        // Create nested struct type
        TypeInfo* point_type = create_type_info(TYPE_STRUCT);
        point_type->struct_name = safe_strdup("Point");
        
        // Create member top_left
        ASTNode* member_tl = create_ast_node(AST_VARIABLE_DECL);
        member_tl->data.variable_decl.name = safe_strdup("top_left");
        member_tl->data.variable_decl.type = point_type;
        
        // Create member bottom_right
        ASTNode* member_br = create_ast_node(AST_VARIABLE_DECL);
        member_br->data.variable_decl.name = safe_strdup("bottom_right");
        member_br->data.variable_decl.type = create_type_info(TYPE_STRUCT);
        member_br->data.variable_decl.type->struct_name = safe_strdup("Point");
        
        member_tl->next = member_br;
        struct_decl->data.struct_decl.members = member_tl;
        
        REQUIRE(struct_decl != nullptr);
        REQUIRE(struct_decl->data.struct_decl.members->data.variable_decl.type->base_type == TYPE_STRUCT);
        
        free_ast_node(struct_decl);
    }
    
    SECTION("test_struct_assignment - should fail initially") {
        // Test struct Point p1 = p2; (memberwise copy)
        CodeGenContext* ctx = create_codegen_context();
        
        // Create struct type
        TypeInfo* struct_type = create_type_info(TYPE_STRUCT);
        struct_type->struct_name = safe_strdup("Point");
        
        // Create two struct variables
        Symbol* p1_symbol = create_symbol();
        p1_symbol->name = safe_strdup("p1");
        p1_symbol->type = struct_type;
        add_local_symbol(ctx, p1_symbol);
        
        Symbol* p2_symbol = create_symbol();
        p2_symbol->name = safe_strdup("p2");
        p2_symbol->type = create_type_info(TYPE_STRUCT);
        p2_symbol->type->struct_name = safe_strdup("Point");
        add_local_symbol(ctx, p2_symbol);
        
        // Create assignment: p1 = p2
        ASTNode* assign_expr = create_ast_node(AST_BINARY_OP);
        assign_expr->data.binary_op.op = BOP_ASSIGN;
        assign_expr->data.binary_op.left = create_identifier_node("p1");
        assign_expr->data.binary_op.right = create_identifier_node("p2");
        
        LLVMValue* result = generate_binary_op(ctx, assign_expr);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        
        free_ast_node(assign_expr);
        free_llvm_value(result);
        free_codegen_context(ctx);
    }
    
    SECTION("test_struct_function_parameters - should fail initially") {
        // Test void func(struct Point p) parameter passing
        TypeInfo* struct_type = create_type_info(TYPE_STRUCT);
        struct_type->struct_name = safe_strdup("Point");
        
        // Create function parameter
        ASTNode* param = create_ast_node(AST_PARAMETER);
        param->data.parameter.name = safe_strdup("p");
        param->data.parameter.type = struct_type;
        
        // Create function declaration
        ASTNode* func_decl = create_ast_node(AST_FUNCTION_DECL);
        func_decl->data.function_decl.name = safe_strdup("test_func");
        func_decl->data.function_decl.return_type = create_type_info(TYPE_VOID);
        func_decl->data.function_decl.parameters = param;
        
        REQUIRE(func_decl != nullptr);
        REQUIRE(func_decl->data.function_decl.parameters->data.parameter.type->base_type == TYPE_STRUCT);
        
        free_ast_node(func_decl);
    }
    
    SECTION("test_array_of_structs - should fail initially") {
        // Test struct Point points[10];
        TypeInfo* struct_type = create_type_info(TYPE_STRUCT);
        struct_type->struct_name = safe_strdup("Point");
        
        TypeInfo* array_type = create_array_type(struct_type, 10);
        
        REQUIRE(array_type != nullptr);
        REQUIRE(array_type->base_type == TYPE_ARRAY);
        REQUIRE(array_type->array_size == 10);
        REQUIRE(array_type->return_type->base_type == TYPE_STRUCT);
        
        free_type_info(array_type);
    }
}

TEST_CASE("Struct + Pointer Combinations") {
    SECTION("test_pointer_to_struct - should fail initially") {
        // Test struct Point* ptr;
        TypeInfo* struct_type = create_type_info(TYPE_STRUCT);
        struct_type->struct_name = safe_strdup("Point");
        
        TypeInfo* ptr_to_struct = create_pointer_type(struct_type);
        
        REQUIRE(ptr_to_struct != nullptr);
        REQUIRE(ptr_to_struct->base_type == TYPE_POINTER);
        REQUIRE(ptr_to_struct->return_type->base_type == TYPE_STRUCT);
        REQUIRE(strcmp(ptr_to_struct->return_type->struct_name, "Point") == 0);
        
        free_type_info(ptr_to_struct);
    }
    
    SECTION("test_struct_with_pointer_members - should fail initially") {
        // Test struct Node { int data; struct Node* next; };
        ASTNode* struct_decl = create_ast_node(AST_STRUCT_DECL);
        struct_decl->data.struct_decl.name = safe_strdup("Node");
        
        // Create data member
        ASTNode* data_member = create_ast_node(AST_VARIABLE_DECL);
        data_member->data.variable_decl.name = safe_strdup("data");
        data_member->data.variable_decl.type = create_type_info(TYPE_INT);
        
        // Create next member (self-referential pointer)
        TypeInfo* node_type = create_type_info(TYPE_STRUCT);
        node_type->struct_name = safe_strdup("Node");
        TypeInfo* ptr_to_node = create_pointer_type(node_type);
        
        ASTNode* next_member = create_ast_node(AST_VARIABLE_DECL);
        next_member->data.variable_decl.name = safe_strdup("next");
        next_member->data.variable_decl.type = ptr_to_node;
        
        data_member->next = next_member;
        struct_decl->data.struct_decl.members = data_member;
        
        REQUIRE(struct_decl != nullptr);
        REQUIRE(struct_decl->data.struct_decl.members->next != nullptr);
        REQUIRE(struct_decl->data.struct_decl.members->next->data.variable_decl.type->base_type == TYPE_POINTER);
        
        free_ast_node(struct_decl);
    }
    
    SECTION("test_dynamic_struct_access - should fail initially") {
        // Test ptr->member->submember nested access
        CodeGenContext* ctx = create_codegen_context();
        
        // Create nested struct types
        TypeInfo* inner_struct = create_type_info(TYPE_STRUCT);
        inner_struct->struct_name = safe_strdup("Inner");
        
        TypeInfo* outer_struct = create_type_info(TYPE_STRUCT);
        outer_struct->struct_name = safe_strdup("Outer");
        
        // Create pointer to outer struct
        Symbol* ptr_symbol = create_symbol();
        ptr_symbol->name = safe_strdup("ptr");
        ptr_symbol->type = create_pointer_type(outer_struct);
        add_local_symbol(ctx, ptr_symbol);
        
        // Create first member access: ptr->inner
        ASTNode* first_access = create_ast_node(AST_MEMBER_ACCESS);
        first_access->data.member_access.object = create_identifier_node("ptr");
        first_access->data.member_access.member = safe_strdup("inner");
        first_access->data.member_access.is_pointer_access = 1;
        
        // Create second member access: (ptr->inner).value
        ASTNode* second_access = create_ast_node(AST_MEMBER_ACCESS);
        second_access->data.member_access.object = first_access;
        second_access->data.member_access.member = safe_strdup("value");
        second_access->data.member_access.is_pointer_access = 0;
        
        LLVMValue* result = generate_member_access(ctx, second_access);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        
        free_ast_node(second_access);
        free_llvm_value(result);
        free_codegen_context(ctx);
    }
    
    SECTION("test_struct_pointer_arithmetic - should fail initially") {
        // Test struct Point* ptr; ptr++; operations
        CodeGenContext* ctx = create_codegen_context();
        
        TypeInfo* struct_type = create_type_info(TYPE_STRUCT);
        struct_type->struct_name = safe_strdup("Point");
        
        // Create pointer to struct array
        Symbol* ptr_symbol = create_symbol();
        ptr_symbol->name = safe_strdup("ptr");
        ptr_symbol->type = create_pointer_type(struct_type);
        add_local_symbol(ctx, ptr_symbol);
        
        // Create increment: ptr++
        ASTNode* inc_expr = create_ast_node(AST_UNARY_OP);
        inc_expr->data.unary_op.op = UOP_POSTINC;
        inc_expr->data.unary_op.operand = create_identifier_node("ptr");
        
        LLVMValue* result = generate_unary_op(ctx, inc_expr);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        REQUIRE(result->llvm_type->base_type == TYPE_POINTER);
        
        free_ast_node(inc_expr);
        free_llvm_value(result);
        free_codegen_context(ctx);
    }
}
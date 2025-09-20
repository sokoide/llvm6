#include "catch2/catch.hpp"

extern "C" {
    #include "../../src/ast.h"
    #include "../../src/codegen.h"
    #include "../../src/memory_management.h"
    #include "../../src/constants.h"
}

TEST_CASE("Struct Declaration Test") {
    // Test struct definition: struct Point { int x; int y; }; - should fail initially
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

TEST_CASE("Struct Member Access Test") {
    // Test obj.member access - should fail initially
    FILE* output = tmpfile();
    CodeGenContext* ctx = create_codegen_context(output);
    
    // Create struct type
    TypeInfo* struct_type = create_type_info(TYPE_STRUCT);
    struct_type->struct_name = safe_strdup("Point");
    
    // Create struct variable: struct Point p;
    Symbol* struct_symbol = create_symbol(safe_strdup("p"), struct_type);
    add_local_symbol(ctx, struct_symbol);
    
    // Create member access: p.x
    ASTNode* member_access = create_ast_node(AST_MEMBER_ACCESS);
    member_access->data.member_access.object = create_identifier_node(safe_strdup("p"));
    member_access->data.member_access.member = safe_strdup("x");
    member_access->data.member_access.is_pointer_access = 0; // dot operator
    
    // This should generate proper member access (will likely fail initially)
    LLVMValue* result = generate_member_access(ctx, member_access);
    
    // We expect this to fail initially or produce stub results
    
    free_ast_node(member_access);
    if (result) free_llvm_value(result);
    free_codegen_context(ctx);
    fclose(output);
}

TEST_CASE("Struct Pointer Access Test") {
    // Test obj->member access - should fail initially
    FILE* output = tmpfile();
    CodeGenContext* ctx = create_codegen_context(output);
    
    // Create struct type
    TypeInfo* struct_type = create_type_info(TYPE_STRUCT);
    struct_type->struct_name = safe_strdup("Point");
    
    // Create pointer to struct: struct Point* ptr;
    TypeInfo* ptr_type = create_pointer_type(struct_type);
    Symbol* ptr_symbol = create_symbol(safe_strdup("ptr"), ptr_type);
    add_local_symbol(ctx, ptr_symbol);
    
    // Create pointer member access: ptr->x
    ASTNode* member_access = create_ast_node(AST_MEMBER_ACCESS);
    member_access->data.member_access.object = create_identifier_node(safe_strdup("ptr"));
    member_access->data.member_access.member = safe_strdup("x");
    member_access->data.member_access.is_pointer_access = 1; // arrow operator
    
    // This should generate proper pointer member access (will likely fail initially)
    LLVMValue* result = generate_member_access(ctx, member_access);
    
    // We expect this to fail initially or produce stub results
    
    free_ast_node(member_access);
    if (result) free_llvm_value(result);
    free_codegen_context(ctx);
    fclose(output);
}

TEST_CASE("Struct Assignment Test") {
    // Test struct Point p1 = p2; (memberwise copy) - should fail initially
    FILE* output = tmpfile();
    CodeGenContext* ctx = create_codegen_context(output);
    
    // Create struct type
    TypeInfo* struct_type1 = create_type_info(TYPE_STRUCT);
    struct_type1->struct_name = safe_strdup("Point");
    
    TypeInfo* struct_type2 = create_type_info(TYPE_STRUCT);
    struct_type2->struct_name = safe_strdup("Point");
    
    // Create two struct variables
    Symbol* p1_symbol = create_symbol(safe_strdup("p1"), struct_type1);
    add_local_symbol(ctx, p1_symbol);
    
    Symbol* p2_symbol = create_symbol(safe_strdup("p2"), struct_type2);
    add_local_symbol(ctx, p2_symbol);
    
    // Create assignment: p1 = p2
    ASTNode* assign_expr = create_ast_node(AST_BINARY_OP);
    assign_expr->data.binary_op.op = OP_ASSIGN;
    assign_expr->data.binary_op.left = create_identifier_node(safe_strdup("p1"));
    assign_expr->data.binary_op.right = create_identifier_node(safe_strdup("p2"));
    
    // This should generate proper struct assignment (will likely fail initially)
    LLVMValue* result = generate_binary_op(ctx, assign_expr);
    
    // We expect this to fail initially or produce incorrect memberwise copy
    
    free_ast_node(assign_expr);
    if (result) free_llvm_value(result);
    free_codegen_context(ctx);
    fclose(output);
}

TEST_CASE("Pointer To Struct Test") {
    // Test struct Point* ptr; - should fail initially
    TypeInfo* struct_type = create_type_info(TYPE_STRUCT);
    struct_type->struct_name = safe_strdup("Point");
    
    TypeInfo* ptr_to_struct = create_pointer_type(struct_type);
    
    REQUIRE(ptr_to_struct != nullptr);
    REQUIRE(ptr_to_struct->base_type == TYPE_POINTER);
    REQUIRE(ptr_to_struct->return_type->base_type == TYPE_STRUCT);
    REQUIRE(strcmp(ptr_to_struct->return_type->struct_name, "Point") == 0);
    
    free_type_info(ptr_to_struct);
}

TEST_CASE("Nested Struct Access Test") {
    // Test ptr->member->submember nested access - should fail initially
    FILE* output = tmpfile();
    CodeGenContext* ctx = create_codegen_context(output);
    
    // Create nested struct types
    TypeInfo* outer_struct = create_type_info(TYPE_STRUCT);
    outer_struct->struct_name = safe_strdup("Outer");
    
    // Create pointer to outer struct
    TypeInfo* ptr_type = create_pointer_type(outer_struct);
    Symbol* ptr_symbol = create_symbol(safe_strdup("ptr"), ptr_type);
    add_local_symbol(ctx, ptr_symbol);
    
    // Create first member access: ptr->inner
    ASTNode* first_access = create_ast_node(AST_MEMBER_ACCESS);
    first_access->data.member_access.object = create_identifier_node(safe_strdup("ptr"));
    first_access->data.member_access.member = safe_strdup("inner");
    first_access->data.member_access.is_pointer_access = 1;
    
    // Create second member access: (ptr->inner).value
    ASTNode* second_access = create_ast_node(AST_MEMBER_ACCESS);
    second_access->data.member_access.object = first_access;
    second_access->data.member_access.member = safe_strdup("value");
    second_access->data.member_access.is_pointer_access = 0;
    
    // This should generate proper nested member access (will likely fail initially)
    LLVMValue* result = generate_member_access(ctx, second_access);
    
    // We expect this to fail initially due to incomplete nested access implementation
    
    free_ast_node(second_access);
    if (result) free_llvm_value(result);
    free_codegen_context(ctx);
    fclose(output);
}
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

TEST_CASE("Struct Features") {
    SECTION("Struct declaration AST creation") {
        ASTNode* struct_node = create_ast_node(AST_STRUCT_DECL);
        REQUIRE(struct_node != nullptr);
        REQUIRE(struct_node->type == AST_STRUCT_DECL);
        free_ast_node(struct_node);
    }

    SECTION("Struct member access (obj.member)") {
        FILE* output = tmpfile();
        REQUIRE(output != nullptr);
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
        
        LLVMValue* result = generate_member_access(ctx, member_access);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        
        free_ast_node(member_access);
        if (result) free_llvm_value(result);
        free_codegen_context(ctx);
        fclose(output);
    }

    SECTION("Struct pointer access (ptr->member)") {
        FILE* output = tmpfile();
        REQUIRE(output != nullptr);
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
        
        LLVMValue* result = generate_member_access(ctx, member_access);
        
        REQUIRE(result != nullptr);
        REQUIRE(result->type == LLVM_VALUE_REGISTER);
        
        free_ast_node(member_access);
        if (result) free_llvm_value(result);
        free_codegen_context(ctx);
        fclose(output);
    }

    SECTION("Pointer to struct types") {
        TypeInfo* struct_type = create_type_info(TYPE_STRUCT);
        struct_type->struct_name = safe_strdup("Point");
        
        TypeInfo* ptr_to_struct = create_pointer_type(struct_type);
        
        REQUIRE(ptr_to_struct != nullptr);
        REQUIRE(ptr_to_struct->base_type == TYPE_POINTER);
        REQUIRE(ptr_to_struct->return_type != nullptr);
        REQUIRE(ptr_to_struct->return_type->base_type == TYPE_STRUCT);
        REQUIRE(strcmp(ptr_to_struct->return_type->struct_name, "Point") == 0);
        
        free_type_info(ptr_to_struct);
    }
}

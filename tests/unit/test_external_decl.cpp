#include "catch2/catch.hpp"

extern "C" {
    #include "../../srccpp/ast.h"
    #include "../../srccpp/codegen.h"
}

TEST_CASE("External Declarations AST") {
    SECTION("Create function declaration node") {
        TypeInfo* ret_type = create_type_info(TYPE_INT);
        ASTNode* param1 = create_variable_decl_node(create_pointer_type(create_type_info(TYPE_CHAR)), "format", NULL);
        
        ASTNode* decl = create_function_decl_node(ret_type, "printf", param1, 1);
        
        REQUIRE(decl != nullptr);
        REQUIRE(decl->type == AST_FUNCTION_DECL);
        REQUIRE(strcmp(decl->data.function_def.name, "printf") == 0);
        REQUIRE(decl->data.function_def.is_variadic == 1);
        REQUIRE(decl->data.function_def.return_type->base_type == TYPE_INT);
        
        free_ast_node(decl);
    }

    SECTION("Create extern variable declaration") {
        TypeInfo* type = create_type_info(TYPE_INT);
        type->storage_class = STORAGE_EXTERN;
        
        ASTNode* decl = create_variable_decl_node(type, "errno", NULL);
        
        REQUIRE(decl != nullptr);
        REQUIRE(decl->type == AST_VARIABLE_DECL);
        REQUIRE(strcmp(decl->data.variable_decl.name, "errno") == 0);
        REQUIRE(decl->data.variable_decl.type->storage_class == STORAGE_EXTERN);
        
        free_ast_node(decl);
    }
}

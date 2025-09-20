#include "catch2/catch.hpp"

extern "C" {
    #include "../../src/ast.h"
    #include "../../src/memory_management.h"
    #include "../../src/constants.h"
}

TEST_CASE("AST Node Creation") {
    SECTION("create_ast_node creates valid node") {
        ASTNode* node = create_ast_node(AST_IDENTIFIER);
        
        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_IDENTIFIER);
        
        free_ast_node(node);
    }
    
    SECTION("create_ast_node with different types") {
        ASTNodeType types[] = {AST_CONSTANT, AST_BINARY_OP, AST_FUNCTION_CALL, AST_RETURN_STMT};
        
        for (auto type : types) {
            ASTNode* node = create_ast_node(type);
            REQUIRE(node != nullptr);
            REQUIRE(node->type == type);
            free_ast_node(node);
        }
    }
}

TEST_CASE("Identifier Node Creation") {
    SECTION("create_identifier_node with valid name") {
        const char* name = "variable_name";
        ASTNode* node = create_identifier_node(name);
        
        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_IDENTIFIER);
        REQUIRE(node->data.identifier.name != nullptr);
        REQUIRE(strcmp(node->data.identifier.name, name) == 0);
        
        free_ast_node(node);
    }
    
    SECTION("create_identifier_node with empty string") {
        ASTNode* node = create_identifier_node("");
        
        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_IDENTIFIER);
        REQUIRE(node->data.identifier.name != nullptr);
        REQUIRE(strcmp(node->data.identifier.name, "") == 0);
        
        free_ast_node(node);
    }
}

TEST_CASE("Constant Node Creation") {
    SECTION("create_constant_node with integer") {
        int value = 42;
        ASTNode* node = create_constant_node(TYPE_INT, &value);
        
        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_CONSTANT);
        REQUIRE(node->data.constant.const_type == TYPE_INT);
        REQUIRE(node->data.constant.value.int_val == 42);
        
        free_ast_node(node);
    }
    
    SECTION("create_constant_node with float") {
        float value = 3.14f;
        ASTNode* node = create_constant_node(TYPE_FLOAT, &value);
        
        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_CONSTANT);
        REQUIRE(node->data.constant.const_type == TYPE_FLOAT);
        REQUIRE(node->data.constant.value.float_val == Catch::Approx(3.14f));
        
        free_ast_node(node);
    }
    
    SECTION("create_constant_node with char") {
        char value = 'A';
        ASTNode* node = create_constant_node(TYPE_CHAR, &value);
        
        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_CONSTANT);
        REQUIRE(node->data.constant.const_type == TYPE_CHAR);
        REQUIRE(node->data.constant.value.char_val == 'A');
        
        free_ast_node(node);
    }
}

TEST_CASE("String Literal Node Creation") {
    SECTION("create_string_literal_node with valid string") {
        const char* str = "Hello, World!";
        ASTNode* node = create_string_literal_node(str);
        
        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_STRING_LITERAL);
        REQUIRE(node->data.string_literal.string != nullptr);
        REQUIRE(strcmp(node->data.string_literal.string, str) == 0);
        REQUIRE(node->data.string_literal.length == strlen(str));
        
        free_ast_node(node);
    }
    
    SECTION("create_string_literal_node with empty string") {
        const char* str = "";
        ASTNode* node = create_string_literal_node(str);
        
        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_STRING_LITERAL);
        REQUIRE(node->data.string_literal.string != nullptr);
        REQUIRE(strcmp(node->data.string_literal.string, "") == 0);
        REQUIRE(node->data.string_literal.length == 0);
        
        free_ast_node(node);
    }
}

TEST_CASE("Binary Operation Node Creation") {
    SECTION("create_binary_op_node with valid operands") {
        ASTNode* left = create_constant_node(TYPE_INT, &(int){5});
        ASTNode* right = create_constant_node(TYPE_INT, &(int){3});
        ASTNode* node = create_binary_op_node(OP_ADD, left, right);
        
        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_BINARY_OP);
        REQUIRE(node->data.binary_op.op == OP_ADD);
        REQUIRE(node->data.binary_op.left == left);
        REQUIRE(node->data.binary_op.right == right);
        
        free_ast_node(node);
    }
    
    SECTION("create_binary_op_node with different operators") {
        BinaryOp ops[] = {OP_SUB, OP_MUL, OP_DIV, OP_LT, OP_GT, OP_EQ};
        
        for (auto op : ops) {
            ASTNode* left = create_constant_node(TYPE_INT, &(int){10});
            ASTNode* right = create_constant_node(TYPE_INT, &(int){20});
            ASTNode* node = create_binary_op_node(op, left, right);
            
            REQUIRE(node != nullptr);
            REQUIRE(node->type == AST_BINARY_OP);
            REQUIRE(node->data.binary_op.op == op);
            
            free_ast_node(node);
        }
    }
}

TEST_CASE("Unary Operation Node Creation") {
    SECTION("create_unary_op_node with valid operand") {
        ASTNode* operand = create_constant_node(TYPE_INT, &(int){42});
        ASTNode* node = create_unary_op_node(UOP_MINUS, operand);
        
        REQUIRE(node != nullptr);
        REQUIRE(node->type == AST_UNARY_OP);
        REQUIRE(node->data.unary_op.op == UOP_MINUS);
        REQUIRE(node->data.unary_op.operand == operand);
        
        free_ast_node(node);
    }
    
    SECTION("create_unary_op_node with different operators") {
        UnaryOp ops[] = {UOP_PLUS, UOP_NOT, UOP_BITNOT, UOP_PREINC, UOP_ADDR};
        
        for (auto op : ops) {
            ASTNode* operand = create_identifier_node("var");
            ASTNode* node = create_unary_op_node(op, operand);
            
            REQUIRE(node != nullptr);
            REQUIRE(node->type == AST_UNARY_OP);
            REQUIRE(node->data.unary_op.op == op);
            
            free_ast_node(node);
        }
    }
}

TEST_CASE("Type Info Creation") {
    SECTION("create_type_info creates valid type") {
        TypeInfo* type = create_type_info(TYPE_INT);
        
        REQUIRE(type != nullptr);
        REQUIRE(type->base_type == TYPE_INT);
        REQUIRE(type->qualifiers == QUAL_NONE);
        REQUIRE(type->storage_class == STORAGE_NONE);
        REQUIRE(type->pointer_level == 0);
        
        free_type_info(type);
    }
    
    SECTION("create_type_info with different base types") {
        DataType types[] = {TYPE_VOID, TYPE_CHAR, TYPE_FLOAT, TYPE_DOUBLE};
        
        for (auto base_type : types) {
            TypeInfo* type = create_type_info(base_type);
            
            REQUIRE(type != nullptr);
            REQUIRE(type->base_type == base_type);
            
            free_type_info(type);
        }
    }
}

TEST_CASE("Pointer Type Creation") {
    SECTION("create_pointer_type creates valid pointer") {
        TypeInfo* base = create_type_info(TYPE_INT);
        TypeInfo* ptr = create_pointer_type(base);
        
        REQUIRE(ptr != nullptr);
        REQUIRE(ptr->base_type == TYPE_POINTER);
        REQUIRE(ptr->pointer_level == 1);
        
        free_type_info(ptr);
    }
    
    SECTION("create_pointer_type nested pointers") {
        TypeInfo* base = create_type_info(TYPE_CHAR);
        TypeInfo* ptr1 = create_pointer_type(base);
        TypeInfo* ptr2 = create_pointer_type(ptr1);
        
        REQUIRE(ptr2 != nullptr);
        REQUIRE(ptr2->base_type == TYPE_POINTER);
        REQUIRE(ptr2->pointer_level == 2);
        
        free_type_info(ptr2);
    }
}

TEST_CASE("Symbol Creation") {
    SECTION("create_symbol creates valid symbol") {
        TypeInfo* type = create_type_info(TYPE_INT);
        Symbol* symbol = create_symbol("test_var", type);
        
        REQUIRE(symbol != nullptr);
        REQUIRE(symbol->name != nullptr);
        REQUIRE(strcmp(symbol->name, "test_var") == 0);
        REQUIRE(symbol->type == type);
        REQUIRE(symbol->is_global == false);
        REQUIRE(symbol->is_parameter == false);
        
        free_symbol(symbol);
    }
}
#include "catch2/catch.hpp"
#include <cstring>

extern "C" {
    #include "../../src/codegen.h"
    #include "../../src/ast.h"
    #include "../../src/constants.h"
}

TEST_CASE("LLVM Type String Conversion") {
    SECTION("llvm_type_to_string handles null type") {
        char* result = llvm_type_to_string(nullptr);
        
        REQUIRE(result != nullptr);
        REQUIRE(strcmp(result, "void") == 0);
        
        free(result);
    }
    
    SECTION("llvm_type_to_string converts basic types correctly") {
        DataType types[] = {TYPE_VOID, TYPE_CHAR, TYPE_SHORT, TYPE_INT, TYPE_LONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER};
        const char* expected[] = {"void", "i8", "i16", "i32", "i64", "float", "double", "i32*"};
        
        for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
            TypeInfo* type = create_type_info(types[i]);
            char* result = llvm_type_to_string(type);
            
            REQUIRE(result != nullptr);
            REQUIRE(strcmp(result, expected[i]) == 0);
            
            free(result);
            free_type_info(type);
        }
    }
    
    SECTION("llvm_type_to_string handles unknown types") {
        TypeInfo* type = create_type_info((DataType)999); // Invalid type
        char* result = llvm_type_to_string(type);
        
        REQUIRE(result != nullptr);
        REQUIRE(strcmp(result, "i32") == 0); // Default fallback
        
        free(result);
        free_type_info(type);
    }
}

TEST_CASE("Register Generation") {
    SECTION("get_next_register generates sequential registers") {
        CodeGenContext* ctx = create_codegen_context();
        
        // Test first few registers
        char* reg1 = get_next_register(ctx);
        char* reg2 = get_next_register(ctx);
        char* reg3 = get_next_register(ctx);
        
        REQUIRE(reg1 != nullptr);
        REQUIRE(reg2 != nullptr);
        REQUIRE(reg3 != nullptr);
        
        REQUIRE(strcmp(reg1, "1") == 0);
        REQUIRE(strcmp(reg2, "2") == 0);
        REQUIRE(strcmp(reg3, "3") == 0);
        
        free(reg1);
        free(reg2);
        free(reg3);
        free_codegen_context(ctx);
    }
    
    SECTION("get_next_register increments correctly") {
        CodeGenContext* ctx = create_codegen_context();
        
        // Generate many registers to test increment
        const int num_regs = 10;
        char* regs[num_regs];
        
        for (int i = 0; i < num_regs; i++) {
            regs[i] = get_next_register(ctx);
            REQUIRE(regs[i] != nullptr);
            
            // Check register number matches expected value
            char expected[16];
            snprintf(expected, sizeof(expected), "%d", i + 1);
            REQUIRE(strcmp(regs[i], expected) == 0);
        }
        
        // Clean up
        for (int i = 0; i < num_regs; i++) {
            free(regs[i]);
        }
        free_codegen_context(ctx);
    }
}

TEST_CASE("Basic Block Generation") {
    SECTION("get_next_basic_block generates sequential blocks") {
        CodeGenContext* ctx = create_codegen_context();
        
        char* block1 = get_next_basic_block(ctx);
        char* block2 = get_next_basic_block(ctx);
        char* block3 = get_next_basic_block(ctx);
        
        REQUIRE(block1 != nullptr);
        REQUIRE(block2 != nullptr);
        REQUIRE(block3 != nullptr);
        
        REQUIRE(strcmp(block1, "bb1") == 0);
        REQUIRE(strcmp(block2, "bb2") == 0);
        REQUIRE(strcmp(block3, "bb3") == 0);
        
        free(block1);
        free(block2);
        free(block3);
        free_codegen_context(ctx);
    }
}

TEST_CASE("Default Value Generation") {
    SECTION("get_default_value returns correct defaults") {
        struct {
            DataType type;
            const char* expected;
        } test_cases[] = {
            {TYPE_VOID, "void"},
            {TYPE_CHAR, "0"},
            {TYPE_SHORT, "0"},
            {TYPE_INT, "0"},
            {TYPE_LONG, "0"},
            {TYPE_FLOAT, "0.0"},
            {TYPE_DOUBLE, "0.0"},
            {TYPE_POINTER, "null"}
        };
        
        for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
            TypeInfo* type = create_type_info(test_cases[i].type);
            char* result = get_default_value(type);
            
            REQUIRE(result != nullptr);
            REQUIRE(strcmp(result, test_cases[i].expected) == 0);
            
            free(result);
            free_type_info(type);
        }
    }
    
    SECTION("get_default_value handles null type") {
        char* result = get_default_value(nullptr);
        
        REQUIRE(result != nullptr);
        REQUIRE(strcmp(result, "void") == 0);
        
        free(result);
    }
    
    SECTION("get_default_value handles unknown types") {
        TypeInfo* type = create_type_info((DataType)999); // Invalid type
        char* result = get_default_value(type);
        
        REQUIRE(result != nullptr);
        REQUIRE(strcmp(result, "0") == 0); // Default fallback
        
        free(result);
        free_type_info(type);
    }
}

TEST_CASE("LLVM Value Creation and Management") {
    SECTION("create_llvm_value creates valid value") {
        LLVMValue* value = create_llvm_value("test_value", TYPE_INT);
        
        REQUIRE(value != nullptr);
        REQUIRE(value->name != nullptr);
        REQUIRE(strcmp(value->name, "test_value") == 0);
        REQUIRE(value->type == TYPE_INT);
        REQUIRE(value->is_register == false);
        REQUIRE(value->is_constant == false);
        
        free_llvm_value(value);
    }
    
    SECTION("create_llvm_value with different types") {
        DataType types[] = {TYPE_CHAR, TYPE_INT, TYPE_FLOAT, TYPE_POINTER};
        
        for (auto type : types) {
            LLVMValue* value = create_llvm_value("test", type);
            
            REQUIRE(value != nullptr);
            REQUIRE(value->type == type);
            
            free_llvm_value(value);
        }
    }
    
    SECTION("create_llvm_value with empty name") {
        LLVMValue* value = create_llvm_value("", TYPE_INT);
        
        REQUIRE(value != nullptr);
        REQUIRE(value->name != nullptr);
        REQUIRE(strcmp(value->name, "") == 0);
        
        free_llvm_value(value);
    }
    
    SECTION("free_llvm_value handles null pointer") {
        // Should not crash
        free_llvm_value(nullptr);
    }
}

TEST_CASE("CodeGen Context Management") {
    SECTION("create_codegen_context creates valid context") {
        CodeGenContext* ctx = create_codegen_context();
        
        REQUIRE(ctx != nullptr);
        REQUIRE(ctx->next_reg_id == 1);
        REQUIRE(ctx->next_bb_id == 1);
        REQUIRE(ctx->output_file != nullptr);
        REQUIRE(ctx->global_symbols == nullptr);
        REQUIRE(ctx->local_symbols == nullptr);
        REQUIRE(ctx->current_function == nullptr);
        
        free_codegen_context(ctx);
    }
    
    SECTION("free_codegen_context handles null pointer") {
        // Should not crash
        free_codegen_context(nullptr);
    }
}

TEST_CASE("Symbol Management") {
    SECTION("add_global_symbol adds symbol correctly") {
        CodeGenContext* ctx = create_codegen_context();
        TypeInfo* type = create_type_info(TYPE_INT);
        
        add_global_symbol(ctx, "global_var", type);
        
        Symbol* found = lookup_symbol(ctx, "global_var");
        REQUIRE(found != nullptr);
        REQUIRE(strcmp(found->name, "global_var") == 0);
        REQUIRE(found->is_global == true);
        
        free_codegen_context(ctx);
    }
    
    SECTION("add_local_symbol adds symbol correctly") {
        CodeGenContext* ctx = create_codegen_context();
        TypeInfo* type = create_type_info(TYPE_INT);
        
        add_local_symbol(ctx, "local_var", type);
        
        Symbol* found = lookup_symbol(ctx, "local_var");
        REQUIRE(found != nullptr);
        REQUIRE(strcmp(found->name, "local_var") == 0);
        REQUIRE(found->is_global == false);
        
        free_codegen_context(ctx);
    }
    
    SECTION("lookup_symbol returns null for non-existent symbol") {
        CodeGenContext* ctx = create_codegen_context();
        
        Symbol* found = lookup_symbol(ctx, "non_existent");
        REQUIRE(found == nullptr);
        
        free_codegen_context(ctx);
    }
    
    SECTION("clear_local_symbols removes local symbols") {
        CodeGenContext* ctx = create_codegen_context();
        TypeInfo* type1 = create_type_info(TYPE_INT);
        TypeInfo* type2 = create_type_info(TYPE_FLOAT);
        TypeInfo* type3 = create_type_info(TYPE_CHAR);
        
        // Add global and local symbols
        add_global_symbol(ctx, "global_var", type1);
        add_local_symbol(ctx, "local_var1", type2);
        add_local_symbol(ctx, "local_var2", type3);
        
        // Verify symbols exist
        REQUIRE(lookup_symbol(ctx, "global_var") != nullptr);
        REQUIRE(lookup_symbol(ctx, "local_var1") != nullptr);
        REQUIRE(lookup_symbol(ctx, "local_var2") != nullptr);
        
        // Clear local symbols
        clear_local_symbols(ctx);
        
        // Global should remain, locals should be gone
        REQUIRE(lookup_symbol(ctx, "global_var") != nullptr);
        REQUIRE(lookup_symbol(ctx, "local_var1") == nullptr);
        REQUIRE(lookup_symbol(ctx, "local_var2") == nullptr);
        
        free_codegen_context(ctx);
    }
}
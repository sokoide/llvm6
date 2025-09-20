#include "catch2/catch.hpp"
#include <cstring>

extern "C" {
    #include "../../src/error_handling.h"
    #include "../../src/constants.h"
}

TEST_CASE("Error Creation") {
    SECTION("create_error with basic message") {
        ErrorContext* error = create_error(ERROR_SYNTAX, "test.c", 42, "test_function", "Test error message");
        
        REQUIRE(error != nullptr);
        REQUIRE(error->type == ERROR_SYNTAX);
        REQUIRE(strcmp(error->file, "test.c") == 0);
        REQUIRE(error->line == 42);
        REQUIRE(strcmp(error->function, "test_function") == 0);
        REQUIRE(error->message != nullptr);
        REQUIRE(strcmp(error->message, "Test error message") == 0);
        
        free_error(error);
    }
    
    SECTION("create_error with formatted message") {
        ErrorContext* error = create_error(ERROR_SEMANTIC, "parser.c", 100, "parse_expression", 
                                         "Undefined variable '%s' at line %d", "variable_name", 25);
        
        REQUIRE(error != nullptr);
        REQUIRE(error->type == ERROR_SEMANTIC);
        REQUIRE(strcmp(error->file, "parser.c") == 0);
        REQUIRE(error->line == 100);
        REQUIRE(strcmp(error->function, "parse_expression") == 0);
        REQUIRE(error->message != nullptr);
        REQUIRE(strcmp(error->message, "Undefined variable 'variable_name' at line 25") == 0);
        
        free_error(error);
    }
    
    SECTION("create_error with different error types") {
        ErrorType types[] = {ERROR_LEXICAL, ERROR_SYNTAX, ERROR_SEMANTIC, ERROR_TYPE_MISMATCH, ERROR_CODEGEN};
        
        for (auto type : types) {
            ErrorContext* error = create_error(type, "file.c", 1, "func", "Error message");
            
            REQUIRE(error != nullptr);
            REQUIRE(error->type == type);
            REQUIRE(error->message != nullptr);
            
            free_error(error);
        }
    }
    
    SECTION("create_error with empty message") {
        ErrorContext* error = create_error(ERROR_INTERNAL, "test.c", 1, "test", "");
        
        REQUIRE(error != nullptr);
        REQUIRE(error->type == ERROR_INTERNAL);
        REQUIRE(error->message != nullptr);
        REQUIRE(strcmp(error->message, "") == 0);
        
        free_error(error);
    }
    
    SECTION("create_error with long message") {
        std::string long_msg(500, 'A'); // 500 character string
        ErrorContext* error = create_error(ERROR_SYNTAX, "test.c", 1, "test", long_msg.c_str());
        
        REQUIRE(error != nullptr);
        REQUIRE(error->message != nullptr);
        REQUIRE(strlen(error->message) <= MAX_TEMP_BUFFER_SIZE - 1);
        
        free_error(error);
    }
}

TEST_CASE("Error Type to String Conversion") {
    SECTION("error_type_to_string returns correct strings") {
        REQUIRE(strcmp(error_type_to_string(ERROR_LEXICAL), "Lexical Error") == 0);
        REQUIRE(strcmp(error_type_to_string(ERROR_SYNTAX), "Syntax Error") == 0);
        REQUIRE(strcmp(error_type_to_string(ERROR_SEMANTIC), "Semantic Error") == 0);
        REQUIRE(strcmp(error_type_to_string(ERROR_TYPE_MISMATCH), "Type Mismatch") == 0);
        REQUIRE(strcmp(error_type_to_string(ERROR_CODEGEN), "Code Generation Error") == 0);
        REQUIRE(strcmp(error_type_to_string(ERROR_INTERNAL), "Internal Error") == 0);
        REQUIRE(strcmp(error_type_to_string(ERROR_MEMORY_ALLOCATION), "Memory Allocation Error") == 0);
        REQUIRE(strcmp(error_type_to_string(ERROR_FILE_IO), "File I/O Error") == 0);
    }
    
    SECTION("error_type_to_string with invalid type") {
        const char* result = error_type_to_string((ErrorType)999);
        REQUIRE(strcmp(result, "Unknown Error") == 0);
    }
}

TEST_CASE("Error Memory Management") {
    SECTION("free_error handles null pointer") {
        // Should not crash
        free_error(nullptr);
    }
    
    SECTION("free_error properly deallocates") {
        ErrorContext* error = create_error(ERROR_SYNTAX, "test.c", 1, "test", "Test message");
        REQUIRE(error != nullptr);
        
        // This should not crash and should properly free memory
        free_error(error);
        
        // Note: We can't verify memory was actually freed without memory debugging tools
        // but we can ensure the function doesn't crash
    }
    
    SECTION("multiple error creation and cleanup") {
        const int num_errors = 10;
        ErrorContext* errors[num_errors];
        
        // Create multiple errors
        for (int i = 0; i < num_errors; i++) {
            errors[i] = create_error(ERROR_SYNTAX, "test.c", i, "test", "Error %d", i);
            REQUIRE(errors[i] != nullptr);
        }
        
        // Free all errors
        for (int i = 0; i < num_errors; i++) {
            free_error(errors[i]);
        }
    }
}
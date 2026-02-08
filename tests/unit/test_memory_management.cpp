#include "catch2/catch.hpp"
#include <cstring>

extern "C" {
    #include "../../srccpp/memory_management.h"
    #include "../../srccpp/constants.h"
}

TEST_CASE("Memory Context Creation") {
    SECTION("create_memory_context creates valid context") {
        MemoryContext* ctx = create_memory_context();
        
        REQUIRE(ctx != nullptr);
        REQUIRE(ctx->total_allocated == 0);
        REQUIRE(ctx->total_freed == 0);
        REQUIRE(ctx->allocation_count == 0);
        REQUIRE(ctx->free_count == 0);
        REQUIRE(ctx->debug_enabled == false);
        REQUIRE(ctx->allocation_list == nullptr);
        
        free_memory_context(ctx);
    }
}

TEST_CASE("Memory Debugging Control") {
    SECTION("enable_memory_debugging sets flag") {
        MemoryContext* ctx = create_memory_context();
        
        enable_memory_debugging(ctx);
        REQUIRE(ctx->debug_enabled == true);
        
        free_memory_context(ctx);
    }
    
    SECTION("disable_memory_debugging clears flag") {
        MemoryContext* ctx = create_memory_context();
        
        enable_memory_debugging(ctx);
        REQUIRE(ctx->debug_enabled == true);
        
        disable_memory_debugging(ctx);
        REQUIRE(ctx->debug_enabled == false);
        
        free_memory_context(ctx);
    }
    
    SECTION("memory debugging toggle") {
        MemoryContext* ctx = create_memory_context();
        
        // Initially disabled
        REQUIRE(ctx->debug_enabled == false);
        
        // Enable
        enable_memory_debugging(ctx);
        REQUIRE(ctx->debug_enabled == true);
        
        // Disable
        disable_memory_debugging(ctx);
        REQUIRE(ctx->debug_enabled == false);
        
        free_memory_context(ctx);
    }
}

TEST_CASE("Safe Memory Allocation") {
    SECTION("safe_malloc_debug allocates memory") {
        MemoryContext* ctx = create_memory_context();
        enable_memory_debugging(ctx);
        
        void* ptr = safe_malloc_debug(ctx, 100, "test.c", 42, "test_function");
        
        REQUIRE(ptr != nullptr);
        REQUIRE(ctx->allocation_count == 1);
        REQUIRE(ctx->total_allocated == 100);
        
        safe_free_debug(ctx, ptr, "test.c", 43, "test_function");
        free_memory_context(ctx);
    }
    
    SECTION("safe_malloc_debug with zero size") {
        MemoryContext* ctx = create_memory_context();
        enable_memory_debugging(ctx);
        
        void* ptr = safe_malloc_debug(ctx, 0, "test.c", 42, "test_function");
        
        REQUIRE(ptr != nullptr); // Should still return valid pointer
        REQUIRE(ctx->allocation_count == 1);
        
        safe_free_debug(ctx, ptr, "test.c", 43, "test_function");
        free_memory_context(ctx);
    }
    
    SECTION("safe_calloc_debug allocates zeroed memory") {
        MemoryContext* ctx = create_memory_context();
        enable_memory_debugging(ctx);
        
        int* ptr = (int*)safe_calloc_debug(ctx, 10, sizeof(int), "test.c", 42, "test_function");
        
        REQUIRE(ptr != nullptr);
        REQUIRE(ctx->allocation_count == 1);
        REQUIRE(ctx->total_allocated == 10 * sizeof(int));
        
        // Check that memory is zeroed
        for (int i = 0; i < 10; i++) {
            REQUIRE(ptr[i] == 0);
        }
        
        safe_free_debug(ctx, ptr, "test.c", 43, "test_function");
        free_memory_context(ctx);
    }
    
    SECTION("safe_strdup_debug duplicates string") {
        MemoryContext* ctx = create_memory_context();
        enable_memory_debugging(ctx);
        
        const char* original = "Hello, World!";
        char* copy = safe_strdup_debug(ctx, original, "test.c", 42, "test_function");
        
        REQUIRE(copy != nullptr);
        REQUIRE(copy != original); // Different memory addresses
        REQUIRE(strcmp(copy, original) == 0);
        REQUIRE(ctx->allocation_count == 1);
        REQUIRE(ctx->total_allocated == strlen(original) + 1);
        
        safe_free_debug(ctx, copy, "test.c", 43, "test_function");
        free_memory_context(ctx);
    }
    
    SECTION("safe_strdup_debug with empty string") {
        MemoryContext* ctx = create_memory_context();
        enable_memory_debugging(ctx);
        
        const char* original = "";
        char* copy = safe_strdup_debug(ctx, original, "test.c", 42, "test_function");
        
        REQUIRE(copy != nullptr);
        REQUIRE(strcmp(copy, "") == 0);
        REQUIRE(ctx->allocation_count == 1);
        REQUIRE(ctx->total_allocated == 1);
        
        safe_free_debug(ctx, copy, "test.c", 43, "test_function");
        free_memory_context(ctx);
    }
}

TEST_CASE("Memory Deallocation") {
    SECTION("safe_free_debug deallocates memory") {
        MemoryContext* ctx = create_memory_context();
        enable_memory_debugging(ctx);
        
        void* ptr = safe_malloc_debug(ctx, 100, "test.c", 42, "test_function");
        REQUIRE(ctx->allocation_count == 1);
        REQUIRE(ctx->free_count == 0);
        
        safe_free_debug(ctx, ptr, "test.c", 43, "test_function");
        REQUIRE(ctx->free_count == 1);
        REQUIRE(ctx->total_freed == 100);
        
        free_memory_context(ctx);
    }
    
    SECTION("safe_free_debug with null pointer") {
        MemoryContext* ctx = create_memory_context();
        enable_memory_debugging(ctx);
        
        // Should not crash
        safe_free_debug(ctx, nullptr, "test.c", 42, "test_function");
        REQUIRE(ctx->free_count == 0);
        
        free_memory_context(ctx);
    }
}

TEST_CASE("Memory Statistics") {
    SECTION("memory statistics track allocations correctly") {
        MemoryContext* ctx = create_memory_context();
        enable_memory_debugging(ctx);
        
        // Allocate some memory
        void* ptr1 = safe_malloc_debug(ctx, 100, "test.c", 42, "test_function");
        void* ptr2 = safe_malloc_debug(ctx, 200, "test.c", 43, "test_function");
        void* ptr3 = safe_calloc_debug(ctx, 5, 20, "test.c", 44, "test_function");
        
        REQUIRE(ctx->allocation_count == 3);
        REQUIRE(ctx->total_allocated == 400); // 100 + 200 + (5*20)
        REQUIRE(ctx->free_count == 0);
        REQUIRE(ctx->total_freed == 0);
        
        // Free some memory
        safe_free_debug(ctx, ptr1, "test.c", 45, "test_function");
        REQUIRE(ctx->allocation_count == 3);
        REQUIRE(ctx->free_count == 1);
        REQUIRE(ctx->total_freed == 100);
        
        safe_free_debug(ctx, ptr2, "test.c", 46, "test_function");
        safe_free_debug(ctx, ptr3, "test.c", 47, "test_function");
        
        REQUIRE(ctx->free_count == 3);
        REQUIRE(ctx->total_freed == 400);
        
        free_memory_context(ctx);
    }
}

TEST_CASE("Memory Leak Detection") {
    SECTION("check_memory_leaks with no leaks") {
        MemoryContext* ctx = create_memory_context();
        enable_memory_debugging(ctx);
        
        void* ptr = safe_malloc_debug(ctx, 100, "test.c", 42, "test_function");
        safe_free_debug(ctx, ptr, "test.c", 43, "test_function");
        
        bool has_leaks = check_memory_leaks(ctx);
        REQUIRE(has_leaks == false);
        
        free_memory_context(ctx);
    }
    
    SECTION("check_memory_leaks with leaks") {
        MemoryContext* ctx = create_memory_context();
        enable_memory_debugging(ctx);
        
        // Allocate without freeing
        safe_malloc_debug(ctx, 100, "test.c", 42, "test_function");
        safe_malloc_debug(ctx, 200, "test.c", 43, "test_function");
        
        bool has_leaks = check_memory_leaks(ctx);
        REQUIRE(has_leaks == true);
        
        free_memory_context(ctx);
    }
}

TEST_CASE("Multiple Allocations and Deallocations") {
    SECTION("stress test with many allocations") {
        MemoryContext* ctx = create_memory_context();
        enable_memory_debugging(ctx);
        
        const int num_allocs = 100;
        void* ptrs[num_allocs];
        
        // Allocate many blocks
        for (int i = 0; i < num_allocs; i++) {
            ptrs[i] = safe_malloc_debug(ctx, 10 + i, "test.c", 42 + i, "test_function");
            REQUIRE(ptrs[i] != nullptr);
        }
        
        REQUIRE(ctx->allocation_count == num_allocs);
        
        // Free all blocks
        for (int i = 0; i < num_allocs; i++) {
            safe_free_debug(ctx, ptrs[i], "test.c", 142 + i, "test_function");
        }
        
        REQUIRE(ctx->free_count == num_allocs);
        REQUIRE(check_memory_leaks(ctx) == false);
        
        free_memory_context(ctx);
    }
}
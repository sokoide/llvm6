#include "memory_management.h"
#include "error_handling.h"
#include <string.h>
#include <stdio.h>

/* Global memory context */
MemoryContext* g_memory_context = NULL;

/* Create memory management context */
MemoryContext* create_memory_context(void) {
    MemoryContext* ctx = (MemoryContext*)malloc(sizeof(MemoryContext));
    if (!ctx) {
        fprintf(stderr, "Fatal: Cannot allocate memory context\n");
        exit(ERROR_MEMORY_ALLOCATION);
    }

    /* Initialize statistics */
    ctx->stats.allocations = 0;
    ctx->stats.deallocations = 0;
    ctx->stats.bytes_allocated = 0;
    ctx->stats.bytes_freed = 0;
    ctx->stats.peak_usage = 0;
    ctx->stats.current_usage = 0;

    ctx->allocated_blocks = NULL;
    ctx->debug_mode = 0;

    return ctx;
}

/* Free memory management context */
void free_memory_context(MemoryContext* ctx) {
    if (!ctx) return;

    /* Free any remaining blocks */
    MemoryBlock* block = ctx->allocated_blocks;
    while (block) {
        MemoryBlock* next = block->next;
        fprintf(stderr, "Memory leak detected: %zu bytes allocated at %s:%d in %s()\n",
                block->size, block->file, block->line, block->function);
        free(block);
        block = next;
    }

    free(ctx);
}

/* Enable memory debugging */
void enable_memory_debugging(MemoryContext* ctx) {
    if (ctx) ctx->debug_mode = 1;
}

/* Disable memory debugging */
void disable_memory_debugging(MemoryContext* ctx) {
    if (ctx) ctx->debug_mode = 0;
}

/* Safe malloc with debugging */
void* safe_malloc_debug(size_t size, const char* file, int line, const char* function) {
    if (!g_memory_context) {
        INIT_MEMORY_MANAGEMENT();
    }

    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Error: Memory allocation failed for %zu bytes at %s:%d in %s()\n",
                size, file, line, function);
        exit(ERROR_MEMORY_ALLOCATION);
    }

    /* Update statistics */
    g_memory_context->stats.allocations++;
    g_memory_context->stats.bytes_allocated += size;
    g_memory_context->stats.current_usage += size;

    if (g_memory_context->stats.current_usage > g_memory_context->stats.peak_usage) {
        g_memory_context->stats.peak_usage = g_memory_context->stats.current_usage;
    }

    /* Track allocation in debug mode */
    if (g_memory_context->debug_mode) {
        MemoryBlock* block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
        if (block) {
            block->address = ptr;
            block->size = size;
            block->file = file;
            block->line = line;
            block->function = function;
            block->next = g_memory_context->allocated_blocks;
            block->prev = NULL;

            if (g_memory_context->allocated_blocks) {
                g_memory_context->allocated_blocks->prev = block;
            }
            g_memory_context->allocated_blocks = block;
        }
    }

    return ptr;
}

/* Safe calloc with debugging */
void* safe_calloc_debug(size_t count, size_t size, const char* file, int line, const char* function) {
    size_t total_size = count * size;
    void* ptr = safe_malloc_debug(total_size, file, line, function);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

/* Safe strdup with debugging */
char* safe_strdup_debug(const char* str, const char* file, int line, const char* function) {
    if (!str) return NULL;

    size_t len = strlen(str) + 1;
    char* new_str = (char*)safe_malloc_debug(len, file, line, function);
    if (new_str) {
        strcpy(new_str, str);
    }
    return new_str;
}

/* Safe free with debugging */
void safe_free_debug(void* ptr, const char* file, int line, const char* function) {
    (void)file;
    (void)line;
    (void)function;

    if (!ptr)
        return;

    if (!g_memory_context) {
        fprintf(stderr, "Warning: Freeing memory without memory context at %s:%d in %s()\n",
                file, line, function);
        free(ptr);
        return;
    }

    size_t freed_size = 0;

    /* Find and remove block from tracking in debug mode */
    if (g_memory_context->debug_mode) {
        MemoryBlock* block = g_memory_context->allocated_blocks;
        while (block) {
            if (block->address == ptr) {
                freed_size = block->size;

                if (block->prev) {
                    block->prev->next = block->next;
                } else {
                    g_memory_context->allocated_blocks = block->next;
                }
                if (block->next) {
                    block->next->prev = block->prev;
                }

                free(block);
                break;
            }
            block = block->next;
        }
    }

    if (freed_size > 0) {
        g_memory_context->stats.bytes_freed += freed_size;
        if (g_memory_context->stats.current_usage >= freed_size) {
            g_memory_context->stats.current_usage -= freed_size;
        } else {
            g_memory_context->stats.current_usage = 0;
        }
    }

    g_memory_context->stats.deallocations++;
    free(ptr);
}

/* Print memory statistics */
void print_memory_stats(const MemoryContext* ctx) {
    if (!ctx) return;

    printf("Memory Statistics:\n");
    printf("  Allocations: %zu\n", ctx->stats.allocations);
    printf("  Deallocations: %zu\n", ctx->stats.deallocations);
    printf("  Bytes allocated: %zu\n", ctx->stats.bytes_allocated);
    printf("  Bytes freed: %zu\n", ctx->stats.bytes_freed);
    printf("  Peak usage: %zu bytes\n", ctx->stats.peak_usage);
    printf("  Current usage: %zu bytes\n", ctx->stats.current_usage);
    printf("  Balance: %ld allocations\n", (long)(ctx->stats.allocations - ctx->stats.deallocations));
}

/* Print memory leaks */
void print_memory_leaks(const MemoryContext* ctx) {
    if (!ctx) return;

    if (ctx->stats.allocations != ctx->stats.deallocations) {
        printf("Memory leaks detected: %ld unfreed allocations\n",
               (long)(ctx->stats.allocations - ctx->stats.deallocations));
    }

    if (ctx->debug_mode && ctx->allocated_blocks) {
        printf("Detailed leak information:\n");
        MemoryBlock* block = ctx->allocated_blocks;
        while (block) {
            printf("  Leak: %zu bytes at %s:%d in %s()\n",
                   block->size, block->file, block->line, block->function);
            block = block->next;
        }
    }
}

/* Check for memory leaks */
int check_memory_leaks(const MemoryContext* ctx) {
    if (!ctx) return 0;
    return (ctx->stats.allocations != ctx->stats.deallocations) ? 1 : 0;
}
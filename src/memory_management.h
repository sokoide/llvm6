#ifndef MEMORY_MANAGEMENT_H
#define MEMORY_MANAGEMENT_H
extern "C" {

#include "constants.h"
#include <stdlib.h>

/* Memory management statistics */
typedef struct MemoryStats {
    size_t allocations;
    size_t deallocations;
    size_t bytes_allocated;
    size_t bytes_freed;
    size_t peak_usage;
    size_t current_usage;
} MemoryStats;

/* Memory block header for tracking */
typedef struct MemoryBlock {
    size_t size;
    const char* file;
    int line;
    const char* function;
    struct MemoryBlock* next;
    struct MemoryBlock* prev;
} MemoryBlock;

/* Memory management context */
typedef struct MemoryContext {
    MemoryStats stats;
    MemoryBlock* allocated_blocks;
    int debug_mode;
} MemoryContext;

/* Global memory context */
extern MemoryContext* g_memory_context;

/* Memory management functions */
MemoryContext* create_memory_context(void);
void free_memory_context(MemoryContext* ctx);
void enable_memory_debugging(MemoryContext* ctx);
void disable_memory_debugging(MemoryContext* ctx);

/* Safe allocation functions */
void* safe_malloc_debug(size_t size, const char* file, int line,
                        const char* function);
void* safe_calloc_debug(size_t count, size_t size, const char* file, int line,
                        const char* function);
void* safe_realloc_debug(void* ptr, size_t size, const char* file, int line,
                         const char* function);
char* safe_strdup_debug(const char* str, const char* file, int line,
                        const char* function);
void safe_free_debug(void* ptr, const char* file, int line,
                     const char* function);

/* Memory leak detection */
void print_memory_stats(const MemoryContext* ctx);
void print_memory_leaks(const MemoryContext* ctx);
int check_memory_leaks(const MemoryContext* ctx);

/* Convenience macros */
#define SAFE_MALLOC(size) safe_malloc_debug(size, __FILE__, __LINE__, __func__)
#define SAFE_CALLOC(count, size)                                               \
    safe_calloc_debug(count, size, __FILE__, __LINE__, __func__)
#define SAFE_REALLOC(ptr, size)                                                \
    safe_realloc_debug(ptr, size, __FILE__, __LINE__, __func__)
#define SAFE_STRDUP(str) safe_strdup_debug(str, __FILE__, __LINE__, __func__)
#define SAFE_FREE(ptr) safe_free_debug(ptr, __FILE__, __LINE__, __func__)

/* Initialize/cleanup macros */
#define INIT_MEMORY_MANAGEMENT()                                               \
    do {                                                                       \
        if (!g_memory_context) {                                               \
            g_memory_context = create_memory_context();                        \
        }                                                                      \
    } while (0)

#define CLEANUP_MEMORY_MANAGEMENT()                                            \
    do {                                                                       \
        if (g_memory_context) {                                                \
            print_memory_leaks(g_memory_context);                              \
            free_memory_context(g_memory_context);                             \
            g_memory_context = NULL;                                           \
        }                                                                      \
    } while (0)

/* Backward compatibility - these will be phased out */
#define safe_malloc(size) SAFE_MALLOC(size)
#define safe_strdup(str) SAFE_STRDUP(str)
}
#endif /* MEMORY_MANAGEMENT_H */
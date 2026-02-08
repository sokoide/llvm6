#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

typedef struct Arena Arena;

/* Create a new arena */
Arena* arena_create(size_t initial_capacity);

/* Allocate memory from the arena */
void* arena_alloc(Arena* arena, size_t size);

/* Reset the arena (deallocates all memory) */
void arena_reset(Arena* arena);

/* Destroy the arena and free all system memory */
void arena_destroy(Arena* arena);

/* Global arena for AST and symbols */
extern Arena* g_compiler_arena;

/* Initialize global memory management */
void mem_init(void);

/* Cleanup global memory management */
void mem_cleanup(void);

/* Helper for safe string duplication in arena */
char* arena_strdup(Arena* arena, const char* str);

#endif /* MEMORY_H */

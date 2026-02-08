#include "memory.h"

struct Arena {
    char* buffer;
    size_t capacity;
    size_t offset;
};

Arena* g_compiler_arena = NULL;

Arena* arena_create(size_t initial_capacity) {
    Arena* arena = (Arena*)malloc(sizeof(Arena));
    if (!arena) return NULL;

    arena->buffer = (char*)malloc(initial_capacity);
    if (!arena->buffer) {
        free(arena);
        return NULL;
    }

    arena->capacity = initial_capacity;
    arena->offset = 0;
    return arena;
}

void* arena_alloc(Arena* arena, size_t size) {
    /* Align to 8 bytes */
    size = (size + 7) & ~7;

    if (arena->offset + size > arena->capacity) {
        size_t new_capacity = arena->capacity * 2;
        while (new_capacity < arena->offset + size) {
            new_capacity *= 2;
        }

        char* new_buffer = (char*)realloc(arena->buffer, new_capacity);
        if (!new_buffer) return NULL;

        arena->buffer = new_buffer;
        arena->capacity = new_capacity;
    }

    void* ptr = &arena->buffer[arena->offset];
    arena->offset += size;
    memset(ptr, 0, size); /* Zero-initialize */
    return ptr;
}

char* arena_strdup(Arena* arena, const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* new_str = (char*)arena_alloc(arena, len + 1);
    if (new_str) {
        strcpy(new_str, str);
    }
    return new_str;
}

void arena_reset(Arena* arena) {
    arena->offset = 0;
}

void arena_destroy(Arena* arena) {
    if (arena) {
        free(arena->buffer);
        free(arena);
    }
}

void mem_init(void) {
    if (!g_compiler_arena) {
        g_compiler_arena = arena_create(1024 * 1024); /* 1MB initial */
    }
}

void mem_cleanup(void) {
    if (g_compiler_arena) {
        arena_destroy(g_compiler_arena);
        g_compiler_arena = NULL;
    }
}

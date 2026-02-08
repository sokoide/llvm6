#include "../../src/memory.h"
#include <assert.h>
#include <stdio.h>

void test_arena_basic() {
    printf("Running test_arena_basic...\n");
    Arena* arena = arena_create(1024);
    assert(arena != NULL);

    void* p1 = arena_alloc(arena, 128);
    assert(p1 != NULL);

    void* p2 = arena_alloc(arena, 256);
    assert(p2 != NULL);
    assert(p1 != p2);

    arena_destroy(arena);
    printf("test_arena_basic passed!\n");
}

int main() {
    test_arena_basic();
    return 0;
}
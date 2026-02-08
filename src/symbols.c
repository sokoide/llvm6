#include "symbols.h"

Symbol* g_global_symbols = NULL;
Symbol* g_local_symbols = NULL;

void symbol_add_global(Symbol* symbol) {
    if (!symbol) return;
    printf("Adding global symbol: %s\n", symbol->name);
    symbol->next = g_global_symbols;
    g_global_symbols = symbol;
}

void symbol_add_local(Symbol* symbol) {
    if (!symbol) return;
    printf("Adding local symbol: %s\n", symbol->name);
    /* Check for duplicate symbols in local scope */
    Symbol* existing = g_local_symbols;
    while (existing) {
        if (existing->name && symbol->name && strcmp(existing->name, symbol->name) == 0) {
            return; /* Already exists */
        }
        existing = existing->next;
    }
    symbol->next = g_local_symbols;
    g_local_symbols = symbol;
}

Symbol* symbol_lookup(const char* name) {
    if (!name) return NULL;

    printf("Looking up: %s... ", name);

    /* Check local symbols first */
    Symbol* symbol = g_local_symbols;
    while (symbol) {
        if (symbol->name && strcmp(symbol->name, name) == 0) {
            printf("Found local\n");
            return symbol;
        }
        symbol = symbol->next;
    }

    /* Then check global symbols */
    symbol = g_global_symbols;
    while (symbol) {
        if (symbol->name && strcmp(symbol->name, name) == 0) {
            printf("Found global\n");
            return symbol;
        }
        symbol = symbol->next;
    }

    printf("Not found\n");
    return NULL;
}

void symbol_clear_locals(void) {
    g_local_symbols = NULL;
}

void symbol_clear_all(void) {
    g_local_symbols = NULL;
    g_global_symbols = NULL;
}

void symbol_init_builtins(void) {
    Symbol* s;
    
    /* __builtin_va_list */
    s = create_symbol("__builtin_va_list", create_pointer_type(create_type_info(TYPE_VOID)));
    s->type->storage_class = STORAGE_TYPEDEF;
    s->is_global = 1;
    symbol_add_global(s);

    /* size_t */
    s = create_symbol("size_t", create_type_info(TYPE_LONG));
    s->type->storage_class = STORAGE_TYPEDEF;
    s->is_global = 1;
    symbol_add_global(s);

    /* FILE (stub as void for now) */
    s = create_symbol("FILE", create_type_info(TYPE_VOID));
    s->type->storage_class = STORAGE_TYPEDEF;
    s->is_global = 1;
    symbol_add_global(s);

    /* __int128 stubs */
    s = create_symbol("__int128_t", create_type_info(TYPE_LONG));
    s->type->storage_class = STORAGE_TYPEDEF;
    s->is_global = 1;
    symbol_add_global(s);

    s = create_symbol("__uint128_t", create_type_info(TYPE_LONG));
    s->type->storage_class = STORAGE_TYPEDEF;
    s->is_global = 1;
    symbol_add_global(s);
}

#include "symbols.h"

Symbol* g_global_symbols = NULL;
Symbol* g_local_symbols = NULL;
Symbol* g_tags = NULL;
TypeInfo* g_all_structs = NULL;

void symbol_add_global(Symbol* symbol) {
    if (!symbol) return;
    symbol->is_global = 1;
    symbol->next = g_global_symbols;
    g_global_symbols = symbol;
}

void symbol_add_local(Symbol* symbol) {
    if (!symbol) return;
    symbol->is_global = 0;
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

    /* Check local symbols first */
    Symbol* symbol = g_local_symbols;
    while (symbol) {
        if ((symbol->name && strcmp(symbol->name, name) == 0) ||
            (symbol->original_name && strcmp(symbol->original_name, name) == 0)) {
            return symbol;
        }
        symbol = symbol->next;
    }

    /* Then check global symbols */
    symbol = g_global_symbols;
    while (symbol) {
        if (symbol->name && strcmp(symbol->name, name) == 0) {
            return symbol;
        }
        symbol = symbol->next;
    }

    return NULL;
}

void tag_add(Symbol* symbol) {
    if (!symbol) return;
    symbol->next = g_tags;
    g_tags = symbol;
}

Symbol* tag_lookup(const char* name) {
    if (!name) return NULL;
    Symbol* symbol = g_tags;
    while (symbol) {
        if (symbol->name && strcmp(symbol->name, name) == 0) {
            return symbol;
        }
        symbol = symbol->next;
    }
    return NULL;
}

void symbol_clear_locals(void) {
    g_local_symbols = NULL;
}

void symbol_clear_all(void) {
    g_local_symbols = NULL;
    g_global_symbols = NULL;
    g_tags = NULL;
}

void symbol_init_builtins(void) {
    Symbol* s;

    /* __builtin_va_list (still needed as a base for va_list) */
    s = create_symbol("__builtin_va_list", create_pointer_type(create_type_info(TYPE_VOID)));
    s->type->storage_class = STORAGE_TYPEDEF;
    s->is_global = 1;
    symbol_add_global(s);
}

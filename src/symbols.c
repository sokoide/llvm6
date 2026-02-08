#include "symbols.h"

Symbol* g_global_symbols = NULL;
Symbol* g_local_symbols = NULL;

void symbol_add_global(Symbol* symbol) {
    symbol->next = g_global_symbols;
    g_global_symbols = symbol;
}

void symbol_add_local(Symbol* symbol) {
    /* Check for duplicate symbols in local scope */
    Symbol* existing = g_local_symbols;
    while (existing) {
        if (strcmp(existing->name, symbol->name) == 0) {
            return; /* Already exists */
        }
        existing = existing->next;
    }
    symbol->next = g_local_symbols;
    g_local_symbols = symbol;
}

Symbol* symbol_lookup(const char* name) {
    /* Check local symbols first */
    Symbol* symbol = g_local_symbols;
    while (symbol) {
        if (strcmp(symbol->name, name) == 0) {
            return symbol;
        }
        symbol = symbol->next;
    }

    /* Then check global symbols */
    symbol = g_global_symbols;
    while (symbol) {
        if (strcmp(symbol->name, name) == 0) {
            return symbol;
        }
        symbol = symbol->next;
    }

    return NULL;
}

void symbol_clear_locals(void) {
    g_local_symbols = NULL; /* Memory is managed by arena */
}

#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "ast.h"

/* Symbol table management functions */
void symbol_add_global(Symbol* symbol);
void symbol_add_local(Symbol* symbol);
Symbol* symbol_lookup(const char* name);
void tag_add(Symbol* symbol);
Symbol* tag_lookup(const char* name);
void symbol_clear_locals(void);
void symbol_clear_all(void);
void symbol_init_builtins(void);

/* Global symbol lists */
extern Symbol* g_global_symbols;
extern Symbol* g_local_symbols;
extern Symbol* g_tags;
extern TypeInfo* g_all_structs;

#endif /* SYMBOLS_H */
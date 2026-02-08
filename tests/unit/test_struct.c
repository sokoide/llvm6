#include "../../src/ast.h"
#include "../../src/symbols.h"
#include <assert.h>
#include <stdio.h>

extern ASTNode* program_ast;
extern int yyparse(void);
extern FILE* yyin;

void test_struct_layout() {
    printf("Running test_struct_layout...\n");
    mem_init();

    const char* code = "struct Point { char c; int x; short y; }; int main() { return 0; }";
    FILE* tmp = fopen("temp_struct.c", "w");
    fputs(code, tmp);
    fclose(tmp);

    yyin = fopen("temp_struct.c", "r");
    assert(yyin != NULL);

    int result = yyparse();
    assert(result == 0);
    
    Symbol* s_point = tag_lookup("Point");
    assert(s_point != NULL);
    TypeInfo* t = s_point->type;
    assert(t->base_type == TYPE_STRUCT);
    
    Symbol* m = t->struct_members;
    assert(m != NULL);
    assert(strcmp(m->name, "c") == 0);
    assert(m->offset == 0);
    
    m = m->next;
    assert(m != NULL);
    assert(strcmp(m->name, "x") == 0);
    assert(m->offset == 4);
    
    m = m->next;
    assert(m != NULL);
    assert(strcmp(m->name, "y") == 0);
    assert(m->offset == 8);
    
    assert(t->size == 12);
    assert(t->alignment == 4);
    
    fclose(yyin);
    remove("temp_struct.c");
    mem_cleanup();
    printf("test_struct_layout passed!\n");
}

void test_nested_struct() {
    printf("Running test_nested_struct...\n");
    mem_init();

    const char* code = "struct Outer { char a; struct Inner { int x; } b; }; int main() { return 0; }";
    FILE* tmp = fopen("temp_nested.c", "w");
    fputs(code, tmp);
    fclose(tmp);

    yyin = fopen("temp_nested.c", "r");
    assert(yyin != NULL);

    int result = yyparse();
    assert(result == 0);
    
    Symbol* s_outer = tag_lookup("Outer");
    assert(s_outer != NULL);
    TypeInfo* t_outer = s_outer->type;
    
    Symbol* m_b = struct_lookup_member(t_outer, "b");
    assert(m_b != NULL);
    assert(m_b->type->base_type == TYPE_STRUCT);
    assert(m_b->type->struct_name != NULL);
    assert(strcmp(m_b->type->struct_name, "Inner") == 0);
    
    /* Layout:
       a: offset 0 (size 1, align 1)
       b: offset 4 (Inner size 4, align 4)
       Total size: 8
    */
    assert(m_b->offset == 4);
    assert(t_outer->size == 8);
    
    fclose(yyin);
    remove("temp_nested.c");
    mem_cleanup();
    printf("test_nested_struct passed!\n");
}

int main() {
    test_struct_layout();
    test_nested_struct();
    return 0;
}

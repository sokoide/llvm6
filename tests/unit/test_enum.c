#include "../../src/ast.h"
#include "../../src/symbols.h"
#include <assert.h>
#include <stdio.h>

extern ASTNode* program_ast;
extern int yyparse(void);
extern FILE* yyin;

void test_enum_parsing() {
    printf("Running test_enum_parsing...\n");
    mem_init();

    const char* code = "enum Color { RED, GREEN = 5, BLUE }; int main() { return BLUE; }";
    FILE* tmp = fopen("temp_enum.c", "w");
    fputs(code, tmp);
    fclose(tmp);

    yyin = fopen("temp_enum.c", "r");
    assert(yyin != NULL);

    int result = yyparse();
    assert(result == 0);
    
    /* Verify RED is 0, GREEN is 5, BLUE is 6 in symbol table */
    Symbol* s_red = symbol_lookup("RED");
    assert(s_red != NULL);
    assert(s_red->is_enum_constant);
    assert(s_red->enum_value == 0);

    Symbol* s_green = symbol_lookup("GREEN");
    assert(s_green != NULL);
    assert(s_green->enum_value == 5);

    Symbol* s_blue = symbol_lookup("BLUE");
    assert(s_blue != NULL);
    assert(s_blue->enum_value == 6);
    
    fclose(yyin);
    remove("temp_enum.c");
    mem_cleanup();
    printf("test_enum_parsing passed!\n");
}

int main() {
    test_enum_parsing();
    return 0;
}
#include "../../src/ast.h"
#include "../../src/symbols.h"
#include <assert.h>
#include <stdio.h>

extern ASTNode* program_ast;
extern int yyparse(void);
extern FILE* yyin;

void test_typedef_parsing() {
    printf("Running test_typedef_parsing...\n");
    mem_init();

    const char* code = "typedef int MyInt; int main() { MyInt x = 42; return x; }";
    FILE* tmp = fopen("temp_typedef.c", "w");
    fputs(code, tmp);
    fclose(tmp);

    yyin = fopen("temp_typedef.c", "r");
    assert(yyin != NULL);

    int result = yyparse();
    assert(result == 0);
    
    Symbol* s_myint = symbol_lookup("MyInt");
    assert(s_myint != NULL);
    assert(s_myint->type != NULL);
    assert(s_myint->type->storage_class == STORAGE_TYPEDEF);
    
    fclose(yyin);
    remove("temp_typedef.c");
    mem_cleanup();
    printf("test_typedef_parsing passed!\n");
}

int main() {
    test_typedef_parsing();
    return 0;
}
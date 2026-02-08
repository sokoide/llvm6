#include "../../src/ast.h"
#include <assert.h>
#include <stdio.h>

extern ASTNode* program_ast;
extern int yyparse(void);
extern FILE* yyin;

void test_ast_construction() {
    printf("Running test_ast_construction...\n");
    mem_init();

    const char* code = "int main() { return 42; }";
    FILE* tmp = fopen("temp_test.c", "w");
    fputs(code, tmp);
    fclose(tmp);

    yyin = fopen("temp_test.c", "r");
    assert(yyin != NULL);

    int result = yyparse();
    assert(result == 0);
    assert(program_ast != NULL);
    assert(program_ast->type == AST_FUNCTION_DEF);

    fclose(yyin);
    remove("temp_test.c");
    mem_cleanup();
    printf("test_ast_construction passed!\n");
}

int main() {
    test_ast_construction();
    return 0;
}
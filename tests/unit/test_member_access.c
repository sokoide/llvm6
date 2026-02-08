#include "../../src/ast.h"
#include "../../src/symbols.h"
#include "../../src/codegen.h"
#include <assert.h>
#include <stdio.h>

extern ASTNode* program_ast;
extern int yyparse(void);
extern FILE* yyin;

void test_member_access_codegen() {
    printf("Running test_member_access_codegen...\n");
    mem_init();
    symbol_init_builtins();

    const char* code = "struct Point { int x; int y; }; int main() { struct Point p; p.x = 10; return p.x; }";
    FILE* tmp = fopen("temp_member.c", "w");
    fputs(code, tmp);
    fclose(tmp);

    yyin = fopen("temp_member.c", "r");
    assert(yyin != NULL);

    int result = yyparse();
    assert(result == 0);
    
    FILE* out = fopen("temp_member.ll", "w");
    codegen_init(out);
    codegen_run(program_ast);
    fclose(out);

    /* Check if temp_member.ll contains getelementptr */
    FILE* in = fopen("temp_member.ll", "r");
    char line[256];
    int found_gep = 0;
    while (fgets(line, sizeof(line), in)) {
        if (strstr(line, "getelementptr %struct.Point, %struct.Point* %p, i32 0, i32 0")) {
            found_gep = 1;
        }
    }
    fclose(in);
    assert(found_gep);

    remove("temp_member.c");
    remove("temp_member.ll");
    mem_cleanup();
    printf("test_member_access_codegen passed!\n");
}

int main() {
    test_member_access_codegen();
    return 0;
}
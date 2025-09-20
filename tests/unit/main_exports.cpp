#include <cstdio>

extern "C" {
#include "../../src/ast.h"

FILE* yyin = NULL;
int yylineno = 1;
char* yytext = const_cast<char*>("");
ASTNode* program_ast = NULL;

int yyparse(void) {
    /* Parser stub: use existing program_ast, report success */
    return 0;
}

int yylex(void) {
    return 0;
}
}

#define main ccompiler_main
#line 1 "src/main.cpp"
#include "../../src/main.cpp"
#undef main

#include <cstdio>

extern "C" {
#include "../../srccpp/ast.h"

FILE* yyin = NULL;
int yylineno = 1;
char* yytext = const_cast<char*>("");
ASTNode* program_ast = NULL;
int yydebug = 0;

int yyparse(void) {
    /* Parser stub: use existing program_ast, report success */
    return 0;
}

int yylex(void) {
    return 0;
}
}

#define main ccompiler_main
#line 1 "srccpp/main.cpp"
#include "../../srccpp/main.cpp"
#undef main

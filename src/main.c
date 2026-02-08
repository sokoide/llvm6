#include "common.h"
#include "memory.h"
#include "error.h"
#include "ast.h"
#include "codegen.h"

extern int yyparse(void);
extern FILE* yyin;
extern ASTNode* program_ast;

int main(int argc, char* argv[]) {
    mem_init();
    codegen_init(stdout);

    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            fatal_error("Cannot open input file: %s", argv[1]);
        }
    } else {
        yyin = stdin;
    }

    if (yyparse() == 0 && program_ast) {
        codegen_run(program_ast);
    } else {
        error_report("Compilation failed due to errors.");
    }

    if (yyin != stdin) fclose(yyin);
    mem_cleanup();
    return error_get_count() > 0 ? 1 : 0;
}

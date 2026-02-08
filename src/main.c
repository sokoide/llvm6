#include "common.h"
#include "memory.h"
#include "error.h"
#include "ast.h"
#include "codegen.h"

extern int yyparse(void);
extern FILE* yyin;
extern ASTNode* program_ast;

int main(int argc, char* argv[]) {
    fprintf(stderr, "DEBUG: main started, argc=%d\n", argc);
    mem_init();
    fprintf(stderr, "DEBUG: mem_init done\n");
    symbol_init_builtins();
    fprintf(stderr, "DEBUG: symbol_init_builtins done\n");
    codegen_init(stdout);

    if (argc > 1) {
        fprintf(stderr, "DEBUG: opening file %s\n", argv[1]);
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            fatal_error("Cannot open input file: %s", argv[1]);
        }
    } else {
        fprintf(stderr, "DEBUG: using stdin\n");
        yyin = stdin;
    }

    fprintf(stderr, "DEBUG: starting yyparse\n");
    if (yyparse() == 0 && program_ast) {
        fprintf(stderr, "DEBUG: yyparse success, starting codegen_run\n");
        codegen_run(program_ast);
        fprintf(stderr, "DEBUG: codegen_run done\n");
    } else {
        error_report("Compilation failed due to errors.");
    }

    if (yyin != stdin) fclose(yyin);
    mem_cleanup();
    return error_get_count() > 0 ? 1 : 0;
}

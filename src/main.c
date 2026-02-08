#include "common.h"
#include "memory.h"
#include "error.h"
#include "ast.h"
#include "codegen.h"

extern int yyparse(void);
extern FILE* yyin;
extern ASTNode* program_ast;

int main(int argc, char* argv[]) {
#ifndef TC1
    // Dump code caused infinite loop
    /*
    if (argc > 1) {
        char* p = argv[1];
        int count = 0;
        while (*p && count < 100) {
            fputc(*p, stderr);
            p++;
            count++;
        }
        fputc('\n', stderr);
    }
    */
    // The original `else` block for `argc > 1` was `fputc('E', stderr); ...`.
    // This block is implicitly removed by the instruction "remove loop dump"
    // and not being part of the new code snippet.
#endif

    mem_init();
    symbol_init_builtins();
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

#include "ast.h"
#include "codegen.h"

#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External declarations from lexer and parser */
extern "C" {
extern FILE* yyin;
extern int yyparse(void);
extern int yylineno;
extern char* yytext;
extern int yylex(void);
}
extern ASTNode* program_ast;

/* Global options */
struct CompilerOptions {
    char* input_file;
    char* output_file;
    int debug_mode;
    int verbose;
    int dump_ast;
    int dump_tokens;
} options = {NULL, NULL, 0, 0, 0, 0};

/* Function prototypes */
void print_usage(const char* program_name);
int parse_arguments(int argc, char* argv[]);
void cleanup_resources(void);
int yyerror(const char* s);

/* Print usage information */
void print_usage(const char* program_name) {
    printf("Usage: %s [options] [input_file]\n", program_name);
    printf("\nOptions:\n");
    printf(
        "  -o, --output FILE      Write LLVM IR to FILE (default: stdout)\n");
    printf("  -d, --debug           Enable debug mode\n");
    printf("  -v, --verbose         Enable verbose output\n");
    printf("  -a, --dump-ast        Dump Abstract Syntax Tree\n");
    printf("  -t, --dump-tokens     Dump lexical tokens\n");
    printf("  -h, --help            Show this help message\n");
    printf("\nExamples:\n");
    printf("  %s program.c -o program.ll\n", program_name);
    printf("  %s -v -a program.c\n", program_name);
    printf("  cat program.c | %s > program.ll\n", program_name);
}

/* Parse command line arguments */
int parse_arguments(int argc, char* argv[]) {
    static struct option long_options[] = {{"output", required_argument, 0,
                                            'o'},
                                           {"debug", no_argument, 0, 'd'},
                                           {"verbose", no_argument, 0, 'v'},
                                           {"dump-ast", no_argument, 0, 'a'},
                                           {"dump-tokens", no_argument, 0, 't'},
                                           {"help", no_argument, 0, 'h'},
                                           {0, 0, 0, 0}};

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "o:dvath", long_options,
                            &option_index)) != -1) {
        switch (c) {
        case 'o':
            options.output_file = optarg;
            break;
        case 'd':
            options.debug_mode = 1;
            break;
        case 'v':
            options.verbose = 1;
            break;
        case 'a':
            options.dump_ast = 1;
            break;
        case 't':
            options.dump_tokens = 1;
            break;
        case 'h':
            print_usage(argv[0]);
            exit(0);
        case '?':
            /* getopt_long already printed an error message */
            return -1;
        default:
            abort();
        }
    }

    /* Handle non-option arguments (input files) */
    if (optind < argc) {
        options.input_file = argv[optind];
    }

    return 0;
}

/* Cleanup allocated resources */
void cleanup_resources(void) {
    if (program_ast) {
        free_ast_node(program_ast);
        program_ast = NULL;
    }

    if (yyin && yyin != stdin) {
        fclose(yyin);
        yyin = NULL;
    }
}

/* Main compiler driver */
int main(int argc, char* argv[]) {
    FILE* output_file = stdout;
    CodeGenContext* ctx = NULL;
    int exit_code = 0;
    int result = 0;

    /* Parse command line arguments */
    if (parse_arguments(argc, argv) != 0) {
        exit_code = 1;
        goto cleanup;
    }

    /* Setup input file */
    if (options.input_file) {
        if (options.verbose) {
            fprintf(stderr, "Reading input from: %s\n", options.input_file);
        }

        yyin = fopen(options.input_file, "r");
        if (!yyin) {
            fprintf(stderr, "Error: Cannot open input file '%s'\n",
                    options.input_file);
            exit_code = 1;
            goto cleanup;
        }
    } else {
        if (options.verbose) {
            fprintf(stderr, "Reading input from stdin\n");
        }
        yyin = stdin;
    }

    /* Setup output file */
    if (options.output_file) {
        if (options.verbose) {
            fprintf(stderr, "Writing output to: %s\n", options.output_file);
        }

        output_file = fopen(options.output_file, "w");
        if (!output_file) {
            fprintf(stderr, "Error: Cannot open output file '%s'\n",
                    options.output_file);
            exit_code = 1;
            goto cleanup;
        }
    } else {
        if (options.verbose) {
            fprintf(stderr, "Writing output to stdout\n");
        }
        output_file = stdout;
    }

    /* Initialize code generation context */
    ctx = create_codegen_context(output_file);
    if (!ctx) {
        fprintf(stderr, "Error: Failed to create code generation context\n");
        exit_code = 1;
        goto cleanup;
    }

    if (options.verbose) {
        fprintf(stderr, "Parsing input...\n");
    }

    result = yyparse();
    if (result != 0) {
        fprintf(stderr, "Error: Parsing failed\n");
        exit_code = 1;
        goto cleanup;
    }

    if (!program_ast) {
        fprintf(stderr, "Error: No AST generated\n");
        exit_code = 1;
        goto cleanup;
    }

    if (options.verbose) {
        fprintf(stderr, "Parsing completed successfully\n");
    }

    /* Dump AST if requested */
    if (options.dump_ast) {
        fprintf(stderr, "\n=== Abstract Syntax Tree ===\n");
        print_ast(program_ast, 0);
        fprintf(stderr, "=== End AST ===\n\n");
    }

    /* Generate LLVM IR */
    if (options.verbose) {
        fprintf(stderr, "Generating LLVM IR...\n");
    }

    generate_llvm_ir(ctx, program_ast);

    if (options.verbose) {
        fprintf(stderr, "LLVM IR generation completed\n");
        fprintf(stderr, "Starting cleanup...\n");
    }

cleanup:
    if (ctx) {
        if (options.verbose) {
            fprintf(stderr, "Freeing codegen context...\n");
        }
        free_codegen_context(ctx);
        ctx = NULL;
    }

    if (options.verbose) {
        fprintf(stderr, "Freeing AST...\n");
    }
    cleanup_resources();

    if (output_file && output_file != stdout) {
        fclose(output_file);
    }

    return exit_code;
}

/* Additional helper functions */
void __attribute__((unused)) compiler_info(void) {
    printf("C to LLVM IR Compiler\n");
    printf("Built with bison and flex\n");
    printf("Supports C language constructs with LLVM IR output\n");
}

/* Debug print function */
void __attribute__((unused)) debug_print(const char* format, ...) {
    if (options.debug_mode) {
        va_list args;
        va_start(args, format);
        fprintf(stderr, "[DEBUG] ");
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
        va_end(args);
    }
}

/* Verbose print function */
void __attribute__((unused)) verbose_print(const char* format, ...) {
    if (options.verbose) {
        va_list args;
        va_start(args, format);
        fprintf(stderr, "[INFO] ");
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
        va_end(args);
    }
}
#include "error_handling.h"

#include <stdlib.h>
#include <string.h>

/* Create an error context with formatted message */
ErrorContext* create_error(ErrorType type, const char* file, int line,
                           const char* function, const char* format, ...) {
    ErrorContext* error =
        static_cast<ErrorContext*>(malloc(sizeof(ErrorContext)));
    if (!error) {
        fprintf(stderr, "Fatal: Cannot allocate memory for error context\n");
        exit(ERROR_MEMORY_ALLOCATION);
    }

    error->type = type;
    error->file = file;
    error->line = line;
    error->function = function;

    /* Allocate and format message */
    auto message = static_cast<char*>(malloc(MAX_TEMP_BUFFER_SIZE));
    if (!message) {
        fprintf(stderr, "Fatal: Cannot allocate memory for error message\n");
        free(error);
        exit(ERROR_MEMORY_ALLOCATION);
    }

    va_list args;
    va_start(args, format);
    vsnprintf(message, MAX_TEMP_BUFFER_SIZE, format, args);
    va_end(args);

    error->message = message;
    return error;
}

/* Free error context */
void free_error(ErrorContext* error) {
    if (error) {
        free(error->message);
        free(error);
    }
}

/* Print error to stderr */
void print_error(const ErrorContext* error) {
    if (!error)
        return;

    fprintf(stderr, "Error [%s]: %s\n", error_type_to_string(error->type),
            error->message);
    fprintf(stderr, "  Location: %s:%d in %s()\n", error->file, error->line,
            error->function);
}

/* Convert error type to string */
const char* error_type_to_string(ErrorType type) {
    switch (type) {
    case ERROR_NONE:
        return "NONE";
    case ERROR_MEMORY:
        return "MEMORY_ALLOCATION";
    case ERROR_IO:
        return "FILE_IO";
    case ERROR_PARSE:
        return "PARSING";
    case ERROR_CODEGEN:
        return "CODE_GENERATION";
    case ERROR_INVALID_ARGUMENT:
        return "INVALID_ARGUMENT";
    case ERROR_SYMBOL_NOT_FOUND:
        return "SYMBOL_NOT_FOUND";
    case ERROR_TYPE_MISMATCH:
        return "TYPE_MISMATCH";
    case ERROR_UNSUPPORTED_OPERATION:
        return "UNSUPPORTED_OPERATION";
    default:
        return "UNKNOWN";
    }
}
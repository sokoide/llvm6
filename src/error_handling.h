#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H
extern "C" {

#include "constants.h"
#include <stdarg.h>
#include <stdio.h>

/* Error types */
typedef enum {
    ERROR_NONE = SUCCESS,
    ERROR_MEMORY = ERROR_MEMORY_ALLOCATION,
    ERROR_IO = ERROR_FILE_IO,
    ERROR_PARSE = ERROR_PARSING,
    ERROR_CODEGEN = ERROR_CODE_GENERATION,
    ERROR_INVALID_ARGUMENT,
    ERROR_SYMBOL_NOT_FOUND,
    ERROR_TYPE_MISMATCH,
    ERROR_UNSUPPORTED_OPERATION
} ErrorType;

/* Error context structure */
typedef struct ErrorContext {
    ErrorType type;
    char* message;
    const char* file;
    int line;
    const char* function;
} ErrorContext;

/* Error handling functions */
ErrorContext* create_error(ErrorType type, const char* file, int line,
                           const char* function, const char* format, ...);
void free_error(ErrorContext* error);
void print_error(const ErrorContext* error);
const char* error_type_to_string(ErrorType type);

/* Convenience macros */
#define CREATE_ERROR(type, format, ...)                                        \
    create_error(type, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define HANDLE_NULL_RETURN(ptr, type, format, ...)                             \
    do {                                                                       \
        if (!(ptr)) {                                                          \
            ErrorContext* error = CREATE_ERROR(type, format, ##__VA_ARGS__);   \
            print_error(error);                                                \
            free_error(error);                                                 \
            return NULL;                                                       \
        }                                                                      \
    } while (0)

#define HANDLE_ERROR_RETURN(condition, type, format, ...)                      \
    do {                                                                       \
        if (condition) {                                                       \
            ErrorContext* error = CREATE_ERROR(type, format, ##__VA_ARGS__);   \
            print_error(error);                                                \
            free_error(error);                                                 \
            return NULL;                                                       \
        }                                                                      \
    } while (0)
}
#endif /* ERROR_HANDLING_H */
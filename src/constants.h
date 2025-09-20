#ifndef CONSTANTS_H
#define CONSTANTS_H

/* Type sizes in bytes */
#define INT_SIZE_BYTES 4
#define POINTER_SIZE_BYTES 8
#define FLOAT_SIZE_BYTES 4
#define DOUBLE_SIZE_BYTES 8

/* Buffer sizes */
#define MAX_REGISTER_NAME_LENGTH 32
#define MAX_BASIC_BLOCK_NAME_LENGTH 32
#define MAX_OPERAND_STRING_LENGTH 64
#define MAX_TEMP_BUFFER_SIZE 1024

/* Default values */
#define DEFAULT_INT_VALUE "0"
#define DEFAULT_FLOAT_VALUE "0.0"
#define DEFAULT_DOUBLE_VALUE "0.0"
#define DEFAULT_POINTER_VALUE "null"

/* LLVM IR instruction prefixes */
#define REGISTER_PREFIX "%"
#define GLOBAL_PREFIX "@"
#define LABEL_SUFFIX ":"

/* Error codes */
#define SUCCESS 0
#define ERROR_MEMORY_ALLOCATION 1
#define ERROR_FILE_IO 2
#define ERROR_PARSING 3
#define ERROR_CODE_GENERATION 4

/* Parser constants */
#define MAX_IDENTIFIER_LENGTH 256
#define MAX_STRING_LITERAL_LENGTH 1024

#endif /* CONSTANTS_H */
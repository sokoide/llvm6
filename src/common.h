#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

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

/* Error codes */
#define SUCCESS 0
#define ERROR_MEMORY_ALLOCATION 1
#define ERROR_FILE_IO 2
#define ERROR_PARSING 3
#define ERROR_CODE_GENERATION 4

#endif /* COMMON_H */

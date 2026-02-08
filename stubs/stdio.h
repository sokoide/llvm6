#ifndef STUB_ALL_H
#define STUB_ALL_H
typedef unsigned long size_t;
typedef void* FILE;
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long int64_t;
typedef unsigned long uint64_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
typedef _Bool bool;
typedef int va_list;
#define NULL ((void*)0)
int printf(const char* format, ...);
int fprintf(FILE* stream, const char* format, ...);
int sprintf(char* str, const char* format, ...);
FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
int fputs(const char* s, FILE* stream);
int fflush(FILE* stream);
void* malloc(unsigned long size);
void* realloc(void* ptr, unsigned long size);
void free(void* ptr);
void* memset(void* s, int c, unsigned long n);
void* memcpy(void* dest, const void* src, unsigned long n);
int strcmp(const char* s1, const char* s2);
int atoi(const char* s);
long strtol(const char* s, char** endptr, int base);
unsigned long strlen(const char* s);
char* strdup(const char* s);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
char* strstr(const char* haystack, const char* needle);
void exit(int status);
#endif

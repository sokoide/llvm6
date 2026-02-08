#ifndef STUB_ALL_H
#define STUB_ALL_H
typedef unsigned long size_t;
typedef void* FILE;
extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;
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
#define true 1
#define false 0
int printf(const char* format, ...);
int fprintf(FILE* stream, const char* format, ...);
int sprintf(char* str, const char* format, ...);
int vfprintf(FILE* stream, const char* format, va_list ap);
FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
int fputs(const char* s, FILE* stream);
int fflush(FILE* stream);
int fgetc(FILE* stream);
char* fgets(char* s, int size, FILE* stream);
void* malloc(unsigned long size);
void* realloc(void* ptr, unsigned long size);
void free(void* ptr);
void* memset(void* s, int c, unsigned long n);
void* memcpy(void* dest, const void* src, unsigned long n);
int memcmp(const void* s1, const void* s2, unsigned long n);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, unsigned long n);
int atoi(const char* s);
long strtol(const char* s, char** endptr, int base);
unsigned long strlen(const char* s);
char* strdup(const char* s);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
char* strchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);
void exit(int status);
int isspace(int c);
int isdigit(int c);
int isalpha(int c);
int isalnum(int c);
#endif

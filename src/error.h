#ifndef ERROR_H
#define ERROR_H

#include "common.h"

void error_report(const char* format, ...);
void fatal_error(const char* format, ...);
int error_get_count(void);

/* For testing: suppress printing to stderr */
void error_suppress_output(bool suppress);

#endif /* ERROR_H */
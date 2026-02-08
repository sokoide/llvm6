#include "error.h"

static int g_error_count = 0;
static bool g_suppress_output = false;

void error_report(const char* format, ...) {
    if (!g_suppress_output) {
        va_list args;
        va_start(args, format);
        fprintf(stderr, "Error: ");
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
        va_end(args);
    }
    g_error_count++;
}

void fatal_error(const char* format, ...) {
    if (!g_suppress_output) {
        va_list args;
        va_start(args, format);
        fprintf(stderr, "Fatal Error: ");
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
        va_end(args);
    }
    exit(1);
}

int error_get_count(void) {
    return g_error_count;
}

void error_suppress_output(bool suppress) {
    g_suppress_output = suppress;
}

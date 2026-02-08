#include "../../src/error.h"
#include <assert.h>
#include <stdio.h>

void test_error_basic() {
    printf("Running test_error_basic...\n");
    assert(error_get_count() == 0);

    /* Suppress output during tests to avoid confusing user with "Error:" messages */
    error_suppress_output(true);

    error_report("Test error 1");
    assert(error_get_count() == 1);

    error_report("Test error 2");
    assert(error_get_count() == 2);

    error_suppress_output(false);

    printf("test_error_basic passed!\n");
}

int main() {
    test_error_basic();
    return 0;
}

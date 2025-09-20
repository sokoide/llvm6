/* Test all statement types */

int test_statements(int n) {
    /* Expression statements */
    n;
    n + 1;
    
    /* Compound statements */
    {
        int local = 5;
        local++;
    }
    
    /* If statements */
    if (n > 0) {
        n = n + 1;
    }
    
    if (n < 10) {
        n = n * 2;
    } else {
        n = n / 2;
    }
    
    /* While loop */
    while (n > 0) {
        n = n - 1;
        if (n == 5) continue;
        if (n == 1) break;
    }
    
    /* Do-while loop */
    do {
        n = n + 1;
    } while (n < 10);
    
    /* For loop */
    for (n = 0; n < 5; n = n + 1) {
        if (n == 3) break;
    }
    
    /* For loop with empty parts */
    for (; n < 10;) {
        n = n + 1;
    }
    
    /* Jump statements */
    if (n == 0) {
        return 42;
    }
    
    return n;
}

/* Test function with no parameters */
int test_no_params() {
    return 123;
}
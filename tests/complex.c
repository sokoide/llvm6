/* Test complex and edge case constructs */

/* Test nested function calls */
int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

int nested_calls() {
    return add(multiply(2, 3), add(4, 5));
}

/* Test complex expressions */
int complex_expressions(int x, int y) {
    /* Deeply nested arithmetic */
    int result1 = ((x + y) * (x - y)) / ((x * 2) + (y / 2));
    
    /* Mixed operators with precedence */
    int result2 = x + y * 2 - x / 3 + y % 4;
    
    /* Simple logical operations */
    int result3 = (x > y);
    
    /* Simple conditional logic */
    int result4 = 0;
    if (x > y) {
        result4 = x * 2;
    } else {
        result4 = y + 10;
    }
    
    return result1 + result2 + result3 + result4;
}

/* Test multiple variable declarations and scoping */
int test_scoping() {
    int x = 10;
    
    {
        int y = 20;
        int x = 30;  /* Shadow outer x */
        
        {
            int z = 40;
            int result = x + y + z;  /* Uses inner x */
            
            if (result > 80) {
                int w = 50;
                result = result + w;
            }
            
            return result;
        }
    }
}

/* Test basic increment/decrement (manual) */
int test_increment_decrement() {
    int x = 10;
    int y = 20;
    
    /* Manual increment/decrement */
    x = x + 1;
    y = y - 1;
    int a = x;
    int b = y;
    
    /* Simple expressions */
    int result = x + y;
    
    return a + b + result;
}

/* Test basic constants (string literals not implemented) */
int test_strings() {
    /* Note: String literals and pointers not fully implemented */
    int result = 42;
    return result;
}

/* Test empty statements and blocks */
int test_empty() {
    ;  /* Empty statement */
    
    {  /* Empty block */
    }
    
    if (1) {
        ;  /* Empty if body */
    }
    
    while (0) {
        ;  /* Empty while body (won't execute) */
    }
    
    /* Simple finite loop */
    int i = 0;
    while (i < 1) {
        i = i + 1;
    }
    
    return 0;
}

/* Main function for testing */
int main() {
    int result = 0;
    
    result = result + nested_calls();
    result = result + complex_expressions(10, 5);
    result = result + test_scoping();
    result = result + test_increment_decrement();
    result = result + test_strings();
    result = result + test_empty();
    
    return result;
}
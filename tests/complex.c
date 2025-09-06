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
    
    /* Logical and bitwise combinations */
    int result3 = (x > y) && (x & y) || (x ^ y);
    
    /* Complex conditional */
    int result4 = x > y ? (x < 100 ? x * 2 : x / 2) : (y > 50 ? y + 10 : y - 10);
    
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

/* Test all increment/decrement variations */
int test_increment_decrement() {
    int x = 10;
    int y = 20;
    
    /* Pre-increment/decrement */
    int a = ++x;
    int b = --y;
    
    /* Post-increment/decrement */
    int c = x++;
    int d = y--;
    
    /* In expressions */
    int result = (++x) + (--y) + (x++) + (y--);
    
    return a + b + c + d + result;
}

/* Test string literals */
int test_strings() {
    char* str1 = "Hello";
    char* str2 = "World";
    char* str3 = "Test with spaces and numbers 123!";
    char* empty = "";
    
    /* Note: String operations may be limited */
    return 42;  /* Placeholder return */
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
    
    for (;;) {  /* Infinite loop structure */
        break;  /* Break immediately */
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
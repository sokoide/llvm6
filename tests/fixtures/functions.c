/* Test different function definition styles */

/* Modern C function with multiple parameters */
int modern_func(int a, int b, int c) {
    return a + b + c;
}

/* Old-style C function */
int old_style_func(x, y)
int x;
int y;
{
    return x * y;
}

/* Function with no parameters - modern style */
int no_params_modern() {
    return 100;
}

/* Function with no parameters - old style */
int no_params_old()
{
    return 200;
}

/* Function that calls other functions */
int caller() {
    int a = modern_func(1, 2, 3);
    int b = old_style_func(4, 5);
    int c = no_params_modern();
    int d = no_params_old();
    return a + b + c + d;
}

/* Recursive function */
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

/* Function with complex expressions */
int complex_math(int x) {
    int result = (x * 2 + 3) * (x - 1);
    result = result / (x + 1);
    return result % 100;
}
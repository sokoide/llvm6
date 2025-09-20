/* Test all supported data types and declarations */

/* Global variables with different types */
int global_int = 42;
char global_char = 'A';
float global_float = 3.14;
double global_double = 2.71828;

/* Storage classes */
static int static_var = 100;
extern int extern_var;

/* Function with different parameter types */
int test_types(int i, char c, float f, double d) {
    /* Local variable declarations */
    int local_int;
    char local_char;
    float local_float;
    double local_double;
    
    /* Initialized declarations */
    int init_int = 10;
    char init_char = 'Z';
    float init_float = 1.5;
    double init_double = 9.99;
    
    /* Multiple declarations in one line */
    int a = 1, b = 2, c = 3;
    
    /* Type conversions and mixed arithmetic */
    int mixed = i + c;
    float float_result = f * i;
    double double_result = d / f;
    
    /* Character operations */
    char char_result = c + 1;
    
    return mixed + float_result + double_result + char_result;
}

/* Test signed/unsigned types */
int test_signed_unsigned() {
    signed int si = -100;
    unsigned int ui = 200;
    signed char sc = -50;
    unsigned char uc = 250;
    
    return si + ui + sc + uc;
}

/* Test void function */
void void_function() {
    int temp = 42;
    temp = temp + 1;
    return;
}

/* Function returning void pointer equivalent (not real void*) */
int test_void_return() {
    void_function();
    return 0;
}
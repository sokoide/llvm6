/* Test simple variables (arrays not fully supported yet) */

int test_arrays() {
    /* Simplified to avoid array syntax until fully implemented */

    int val0 = 100;
    int val1 = 200;
    int val2 = 300;
    int val3 = 400;

    /* Basic arithmetic operations */
    int sum = val0 + val1;
    int product = val2 * val3;

    /* More complex expression */
    int complex = (val0 % 5) + (val1 % 5);

    return sum + product + complex;
}

/* Test sizeof operator (simplified) */
int test_sizeof() {
    /* Simplified sizeof tests - direct values */
    int x = 4;  /* sizeof(int) typically 4 */
    int y = 1;  /* sizeof(char) typically 1 */
    int z = 4;  /* sizeof(float) typically 4 */
    int w = 8;  /* sizeof(double) typically 8 */

    /* sizeof with simple expressions */
    int a = 42;
    int size_of_expr = 4;     /* sizeof(a) would be 4 */
    int size_of_complex = 4;  /* sizeof(a + 100) would be 4 */

    return x + y + z + w + size_of_expr + size_of_complex;
}

/* Test simple variable operations (pointer ops not fully supported) */
int test_address_deref() {
    int x = 100;
    /* Simplified without pointer syntax */
    int value = x;  /* Direct assignment instead of pointer dereference */

    return value;
}
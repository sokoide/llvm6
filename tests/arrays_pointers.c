/* Test array access and pointer operations (basic support) */

int test_arrays() {
    /* Note: Full pointer/array support may be limited in current implementation */
    
    int arr[10];  /* Array declaration */
    int i = 0;
    
    /* Array access patterns */
    arr[0] = 100;
    arr[1] = 200;
    arr[i] = 300;
    arr[i + 1] = 400;
    
    /* Array access in expressions */
    int sum = arr[0] + arr[1];
    int product = arr[i] * arr[i + 1];
    
    /* Multiple array accesses */
    int complex = arr[arr[0] % 5] + arr[arr[1] % 5];
    
    return sum + product + complex;
}

/* Test sizeof operator */
int test_sizeof() {
    int x = sizeof(int);
    int y = sizeof(char);
    int z = sizeof(float);
    int w = sizeof(double);
    
    /* sizeof with expressions */
    int a = 42;
    int size_of_expr = sizeof(a);
    int size_of_complex = sizeof(a + 100);
    
    return x + y + z + w + size_of_expr + size_of_complex;
}

/* Test address and dereference operators (basic) */
int test_address_deref() {
    int x = 100;
    int* ptr;  /* Pointer declaration (if supported) */
    
    /* Address operator */
    ptr = &x;
    
    /* Dereference operator */
    int value = *ptr;
    
    return value;
}
/* Demo 15: Array Initialization List */
extern int printf(char *format, ...);

int main() {
    printf("=== Array Initialization Demo ===\n");

    int arr[3] = {10, 20, 30};
    int i;
    for (i = 0; i < 3; i = i + 1) {
        printf("arr[%d] = %d\n", i, arr[i]);
    }

    /* Partial initialization */
    int params[5] = {1, 2};
    /* Remaining elements are uninitialized (garbage) in local stack */
    printf("params[0] = %d\n", params[0]);
    printf("params[1] = %d\n", params[1]);

    /* Char array */
    char s[4] = {'a', 'b', 'c', 0};
    printf("s = %s\n", s);

    return 0;
}

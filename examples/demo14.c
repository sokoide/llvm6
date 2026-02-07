/* Demo 14: _Bool type */
extern int printf(char *format, ...);

int main() {
    _Bool b;

    printf("=== _Bool Type Demo ===\n");

    b = 1;
    printf("b = %d\n", b);

    b = 0;
    printf("b = %d\n", b);

    if (b) {
        printf("b is true\n");
    } else {
        printf("b is false\n");
    }

    /* Integer to bool conversion */
    b = 123; /* should be 1, but might be truncated if not handled */
    printf("b = %d\n", b);

    b = 2; /* should be 1 */
    printf("b=2 -> %d\n", b);

    return 0;
}

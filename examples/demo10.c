/* Demo 10: キャスト式 */

extern int printf(char *format, ...);

int main() {
    int a;
    int b;
    int result;
    char c;

    printf("=== Cast Expression Demo ===\n");

    /* Integer division and cast */
    a = 10;
    b = 3;
    result = a / b;
    printf("%d / %d = %d\n", a, b, result);

    /* Cast to char */
    a = 65;  /* ASCII 'A' */
    c = (char)a;
    printf("ASCII %d = %c\n", a, c);

    /* Cast in expression */
    a = 256 + 66;  /* 322 */
    c = (char)a;   /* Should truncate to 66 = 'B' */
    printf("Truncated: %d -> %c\n", a, c);

    return 0;
}

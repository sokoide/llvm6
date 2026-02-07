/* Demo 11: 条件演算子 ?: */

extern int printf(char *format, ...);

int main() {
    int a;
    int b;
    int max;
    int min;

    printf("=== Conditional Operator Demo ===\n");

    a = 10;
    b = 20;

    max = (a > b) ? a : b;
    min = (a < b) ? a : b;

    printf("a = %d, b = %d\n", a, b);
    printf("max = %d\n", max);
    printf("min = %d\n", min);

    /* 別のテスト */
    a = 5;
    b = 5;
    max = (a > b) ? a : b;
    printf("a = b = 5, max = %d\n", max);

    return 0;
}

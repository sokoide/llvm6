/* Demo 5: 条件演算子と複雑な制御フロー */

extern int printf(char *format, ...);

int main() {
    int a;
    int b;
    int max;
    int i;

    printf("=== Conditional Operator and Loops Demo ===\n");

    /* 条件演算子(?:) */
    a = 10;
    b = 20;
    max = (a > b) ? a : b;
    printf("max of %d and %d is %d\n", a, b, max);

    a = 30;
    max = (a > b) ? a : b;
    printf("max of %d and %d is %d\n", a, b, max);

    /* 入れ子の条件演算子 */
    a = 15;
    b = 25;
    max = (a > b) ? a : ((b > 20) ? 100 : b);
    printf("nested conditional: %d\n", max);

    /* do-whileループ */
    printf("\ndo-while loop:\n");
    i = 0;
    do {
        printf("  i = %d\n", i);
        i = i + 1;
    } while (i < 3);

    /* 複雑なforループ */
    printf("\nfor loop with multiple variables:\n");
    for (i = 0; i < 5; i = i + 1) {
        if (i == 2) {
            printf("  skipping %d\n", i);
            continue;
        }
        if (i == 4) {
            printf("  breaking at %d\n", i);
            break;
        }
        printf("  i = %d\n", i);
    }

    return 0;
}

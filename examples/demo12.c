/* Demo 12: C99 for ループ内変数宣言 */

extern int printf(char *format, ...);

int main() {
    int sum;

    printf("=== C99 For Loop Demo ===\n");

    sum = 0;
    for (int i = 0; i < 5; i = i + 1) {
        sum = sum + i;
        printf("i = %d, sum = %d\n", i, sum);
    }

    printf("Final sum: %d\n", sum);

    /* ネストしたC99 forループ */
    for (int x = 0; x < 3; x = x + 1) {
        for (int y = 0; y < 2; y = y + 1) {
            printf("(%d, %d) ", x, y);
        }
        printf("\n");
    }

    return 0;
}

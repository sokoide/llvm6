/* Demo 7: 配列の基本操作 */

extern int printf(char *format, ...);

int main() {
    int arr[5];
    int i;
    int sum;

    printf("=== Array Demo ===\n");

    /* 配列の初期化 */
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;
    arr[4] = 50;

    /* 配列要素の表示 */
    printf("Array elements:\n");
    for (i = 0; i < 5; i = i + 1) {
        printf("  arr[%d] = %d\n", i, arr[i]);
    }

    /* 配列の合計計算 */
    sum = 0;
    for (i = 0; i < 5; i = i + 1) {
        sum = sum + arr[i];
    }
    printf("Sum of array: %d\n", sum);

    /* 配列要素の変更 */
    arr[2] = 100;
    printf("After arr[2] = 100: arr[2] = %d\n", arr[2]);

    return 0;
}

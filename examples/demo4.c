/* Demo 4: 配列とポインタ操作 */

extern int printf(char *format, ...);

int main() {
    int arr0;
    int arr1;
    int arr2;
    int arr3;
    int arr4;
    int i;

    printf("=== Array and Pointer Demo ===\n");

    /* 配列の初期化 (個別変数として) */
    arr0 = 10;
    arr1 = 20;
    arr2 = 30;
    arr3 = 40;
    arr4 = 50;

    /* 要素を表示 */
    printf("Array elements:\n");
    for (i = 0; i < 5; i = i + 1) {
        if (i == 0) {
            printf("  arr[%d] = %d\n", i, arr0);
        }
        if (i == 1) {
            printf("  arr[%d] = %d\n", i, arr1);
        }
        if (i == 2) {
            printf("  arr[%d] = %d\n", i, arr2);
        }
        if (i == 3) {
            printf("  arr[%d] = %d\n", i, arr3);
        }
        if (i == 4) {
            printf("  arr[%d] = %d\n", i, arr4);
        }
    }

    return 0;
}

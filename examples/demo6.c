/* Demo 6: 関数、ポインタ演算、複合代入演算子 */

extern int printf(char *format, ...);

int add(int a, int b) {
    return a + b;
}

int main() {
    int sum;
    int i;

    printf("=== Function, Pointer, and Compound Assignment Demo ===\n");

    /* 関数呼び出し */
    sum = add(5, 3);
    printf("add(5, 3) = %d\n", sum);

    /* 複合代入演算子 */
    i = 10;
    i += 5;
    printf("i += 5: %d\n", i);
    i *= 2;
    printf("i *= 2: %d\n", i);
    i -= 3;
    printf("i -= 3: %d\n", i);

    /* ビット演算 */
    i = 0x0F;
    i = i & 0x0A;
    printf("i & 0x0A: %d\n", i);
    i = i | 0x20;
    printf("i | 0x20: %d\n", i);
    i = i ^ 0x30;
    printf("i ^ 0x30: %d\n", i);

    /* シフト演算 */
    i = 8;
    i = i << 2;
    printf("i << 2: %d\n", i);
    i = i >> 1;
    printf("i >> 1: %d\n", i);

    return 0;
}

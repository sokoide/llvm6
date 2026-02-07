/* Demo 13: C99 単一行コメント */

extern int printf(char *format, ...);

int main() {
    int a;  // 変数宣言

    printf("=== C99 Single-line Comment Demo ===\n");

    // これは単一行コメント
    a = 42;
    printf("a = %d\n", a);  // 行末コメント

    // 複数の連続コメント
    // もOK
    a = a + 8;
    printf("a + 8 = %d\n", a);

    return 0;  // 終了
}

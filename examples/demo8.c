/* Demo 8: グローバル変数初期化 */

extern int printf(char *format, ...);

int counter = 100;
int multiplier = 5;
int result;

int main() {
    printf("=== Global Variable Initialization Demo ===\n");

    printf("counter = %d\n", counter);
    printf("multiplier = %d\n", multiplier);

    result = counter * multiplier;
    printf("result = %d\n", result);

    /* グローバル変数の変更 */
    counter = counter + 50;
    printf("counter after +50: %d\n", counter);

    return 0;
}

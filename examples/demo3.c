extern int printf(char *format, ...);

int main() {
    int i;

    printf("Testing logical operators and loops\n");

    /* Test ! (NOT) */
    if (!0) {
        printf("!0 is true\n");
    }

    /* Test && (AND) */
    i = 1;
    if (i && 0) {
        printf("Should not print\n");
    } else {
        printf("1 && 0 is false\n");
    }

    /* Test || (OR) */
    if (i || 0) {
        printf("1 || 0 is true\n");
    }

    /* Simple for loop */
    printf("Simple for loop:\n");
    for (i = 0; i < 3; i = i + 1) {
        printf("  i = %d\n", i);
    }

    return 0;
}
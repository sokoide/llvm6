extern int printf(char *format, ...);

int main() {
    int i;
    int j;

    printf("Testing logical operators and loop control\n");

    /* Test ! (NOT) */
    if (!0) {
        printf("!0 is true\n");
    }

    /* Test && (AND) and || (OR) with short-circuit */
    i = 1;
    j = 0;
    if (i && j) {
        printf("Should not print (1 && 0)\n");
    } else {
        printf("1 && 0 is false\n");
    }

    if (i || j) {
        printf("1 || 0 is true\n");
    }

    /* Test break and continue */
    printf("Testing loop control (break/continue):\n");
    for (i = 0; i < 10; i = i + 1) {
        if (i == 2) {
            printf("  Skip 2 (continue)\n");
            continue;
        }
        if (i == 5) {
            printf("  Stop at 5 (break)\n");
            break;
        }
        printf("  Value: %d\n", i);
    }

    return 0;
}
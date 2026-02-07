int printf(char *fmt, ...);

char g1[] = "Global Auto";
char g2[15] = "Global Fixed";

int main() {
    char l1[] = "Local Auto";
    char l2[15] = "Local Fixed";

    printf("=== String Initialization Demo ===\n");

    printf("g1: %s (size: %d)\n", g1, sizeof(g1));
    printf("g2: %s (size: %d)\n", g2, sizeof(g2));
    printf("l1: %s (size: %d)\n", l1, sizeof(l1));
    printf("l2: %s (size: %d)\n", l2, sizeof(l2));

    /* Modify and print */
    l1[0] = 'M';
    printf("l1 modified: %s\n", l1);

    /* Check padding */
    if (l2[11] == 0 && l2[14] == 0) {
        printf("l2 padding verified\n");
    } else {
        printf("l2 padding failed: %d %d\n", l2[11], l2[14]);
    }

    return 0;
}

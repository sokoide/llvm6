#include <stdio.h>

char g1[] = "Global";

int main() {
    char l1[] = "Local";
    printf("l1: %s (size: %d)\n", l1, sizeof(l1));
    return 0;
}

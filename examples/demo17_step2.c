#include <stdio.h>

char g1[] = "Global";

int main() {
    char l1[] = "Local";
    int sz = sizeof(l1);
    printf("l1: %s (size: %d)\n", l1, sz);
    return 0;
}

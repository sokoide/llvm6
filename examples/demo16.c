int printf(char* fmt, ...);

int main() {
    printf("=== Multi-dimensional Array Demo ===\n");

    int arr[2][3];
    int i;
    int j;

    // Initialization loop
    for (i = 0; i < 2; i = i + 1) {
        for (j = 0; j < 3; j = j + 1) {
            arr[i][j] = (i + 1) * 10 + (j + 1);
        }
    }

    // Print loop
    for (i = 0; i < 2; i = i + 1) {
        for (j = 0; j < 3; j = j + 1) {
            printf("arr[%d][%d] = %d\n", i, j, arr[i][j]);
        }
    }

    return 0;
}

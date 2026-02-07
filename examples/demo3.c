extern int printf(char *format, ...);
extern int scanf(char *format, ...);

int main() {
    int a;
    int b;
    printf("Enter two numbers: ");
    scanf("%d %d", &a, &b);
    printf("%d + %d = %d
", a, b, a + b);
    return 0;
}

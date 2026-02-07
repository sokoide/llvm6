/* Demo 9: switch/case文 */

extern int printf(char *format, ...);

int main() {
    int day;
    int i;

    printf("=== Switch/Case Demo ===\n");

    for (i = 1; i <= 4; i = i + 1) {
        day = i;
        printf("Day %d: ", day);

        switch (day) {
            case 1:
                printf("Monday\n");
                break;
            case 2:
                printf("Tuesday\n");
                break;
            case 3:
                printf("Wednesday\n");
                break;
            default:
                printf("Other day\n");
                break;
        }
    }

    return 0;
}

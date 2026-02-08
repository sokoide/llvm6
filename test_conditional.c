/* Test conditional expression type promotion */
int main() {
    int x;
    unsigned char y = 10;
    x = (y < 100) ? y : 2;
    return x;
}

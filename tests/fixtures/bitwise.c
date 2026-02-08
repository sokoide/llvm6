int test_bitwise() {
    int a = 5;     // 0101
    int b = 3;     // 0011
    int c = a & b; // 0001 (1)
    int d = a | b; // 0111 (7)
    int e = a ^ b; // 0110 (6)
    int f = ~a;    // -6
    int g = a << 1; // 1010 (10)
    int h = a >> 1; // 0010 (2)
    return c + d + e + g + h;
}

int main() {
    return test_bitwise();
}

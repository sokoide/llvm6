/* Test unsigned typedef chain resolution */
typedef unsigned char uint8_t;
typedef uint8_t flex_uint8_t;
typedef flex_uint8_t YY_CHAR;

YY_CHAR x = 42;

int main() {
    YY_CHAR y = x + 1;
    return y;
}

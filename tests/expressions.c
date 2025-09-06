/* Test comprehensive expression types */

int test_expressions() {
    /* Primary expressions */
    int x = 42;
    int y = x;
    char* str = "hello";
    int z = (x + y);
    
    /* Arithmetic operators */
    int add = x + y;
    int sub = x - y;
    int mul = x * y;
    int div = x / y;
    int mod = x % y;
    
    /* Comparison operators */
    int lt = x < y;
    int gt = x > y;
    int le = x <= y;
    int ge = x >= y;
    int eq = x == y;
    int ne = x != y;
    
    /* Bitwise operators */
    int and_op = x & y;
    int or_op = x | y;
    int xor_op = x ^ y;
    int lshift = x << 2;
    int rshift = x >> 2;
    
    /* Logical operators */
    int logical_and = x && y;
    int logical_or = x || y;
    
    /* Unary operators */
    int plus = +x;
    int minus = -x;
    int not_op = !x;
    int bitnot = ~x;
    
    /* Assignment operators */
    x = 10;
    x += 5;
    x -= 3;
    x *= 2;
    x /= 4;
    x %= 3;
    x <<= 1;
    x >>= 1;
    x &= 0xFF;
    x |= 0x0F;
    x ^= 0x55;
    
    /* Conditional operator */
    int cond = x > y ? x : y;
    
    /* Comma operator */
    int comma = (x++, y++, x + y);
    
    return x + y;
}
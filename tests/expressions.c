/* Test basic expression types that work with current implementation */

int test_expressions() {
    /* Primary expressions */
    int x = 42;
    int y = 10;
    int z = (x + y);
    
    /* Basic arithmetic operators */
    int add = x + y;
    int sub = x - y;
    int mul = x * y;
    int div = x / y;
    int mod = x % y;
    
    /* Basic comparison operators */
    int lt = x < y;
    int gt = x > y;
    int le = x <= y;
    int ge = x >= y;
    int eq = x == y;
    int ne = x != y;
    
    /* Basic assignment */
    x = 100;
    y = 200;
    
    return x + y + z;
}

int main() {
    return test_expressions();
}
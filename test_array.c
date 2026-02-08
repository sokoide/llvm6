/* Test incomplete array type */
static const short arr[] = {1, 2, 3, 4, 5};
int main() {
    int x = arr[0] + arr[1];
    return x;
}

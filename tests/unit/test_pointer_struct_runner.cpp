#include <iostream>

// External test functions
extern void run_pointer_tests();
extern void run_struct_tests();

// External counters
extern int pointer_tests_failed;
extern int struct_tests_failed;

int main() {
    std::cout << "\n============================================\n";
    std::cout << "Running Pointer and Struct Feature Tests\n";
    std::cout << "============================================\n";
    std::cout << "All pointer and struct feature tests should pass now.\n";
    std::cout << "============================================\n";

    run_pointer_tests();
    run_struct_tests();
    
    std::cout << "\n============================================\n";
    std::cout << "Test Summary:\n";
    if (pointer_tests_failed == 0 && struct_tests_failed == 0) {
        std::cout << "All pointer and struct feature tests passed successfully.\n";
    } else {
        std::cout << "Outstanding pointer/struct issues remain.\n";
    }
    std::cout << "============================================\n";
    
    return (pointer_tests_failed == 0 && struct_tests_failed == 0) ? 0 : 1;
}
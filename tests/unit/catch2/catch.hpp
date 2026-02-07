// Single-header Catch2 library (simplified version for this setup)
// For production use, download the full catch.hpp from https://github.com/catchorg/Catch2

#ifndef CATCH_HPP_INCLUDED
#define CATCH_HPP_INCLUDED

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include <cstring>

// Basic test framework implementation
namespace Catch {
    struct TestCase {
        std::string name;
        std::string file;
        int line;
        void (*func)();
        
        TestCase(const std::string& n, const std::string& f, int l, void (*fn)())
            : name(n), file(f), line(l), func(fn) {}
    };
    
    class TestRegistry {
    public:
        static TestRegistry& instance() {
            static TestRegistry reg;
            return reg;
        }
        
        void add(const TestCase& test) {
            tests.push_back(test);
        }
        
        int run() {
            int passed = 0, failed = 0;
            std::cout << "Running " << tests.size() << " test cases...\n\n";
            
            for (const auto& test : tests) {
                std::cout << "Test: " << test.name << " ... ";
                try {
                    test.func();
                    std::cout << "PASSED\n";
                    passed++;
                } catch (const std::exception& e) {
                    std::cout << "FAILED\n  " << e.what() << "\n";
                    failed++;
                } catch (...) {
                    std::cout << "FAILED\n  Unknown exception\n";
                    failed++;
                }
            }
            
            std::cout << "\nResults: " << passed << " passed, " << failed << " failed\n";
            return failed;
        }
        
    private:
        std::vector<TestCase> tests;
    };
    
    class AssertionException : public std::exception {
    public:
        AssertionException(const std::string& msg) : message(msg) {}
        const char* what() const noexcept override { return message.c_str(); }
    private:
        std::string message;
    };
    
    template<typename T>
    class Approx {
    public:
        Approx(T val) : value(val), epsilon(std::numeric_limits<T>::epsilon() * 100) {}
        
        bool operator==(T other) const {
            return std::abs(value - other) <= epsilon;
        }
        
        Approx& margin(T m) { epsilon = m; return *this; }
        
    private:
        T value;
        T epsilon;
    };
}

#define CATCH_INTERNAL_CONCAT2(a, b) a##b
#define CATCH_INTERNAL_CONCAT(a, b) CATCH_INTERNAL_CONCAT2(a, b)

#define TEST_CASE(name) \
    static void CATCH_INTERNAL_CONCAT(CATCH_INTERNAL_TestFunction, __LINE__)(); \
    namespace { \
        struct CATCH_INTERNAL_CONCAT(CATCH_INTERNAL_TestRegistrar, __LINE__) { \
            CATCH_INTERNAL_CONCAT(CATCH_INTERNAL_TestRegistrar, __LINE__)() { \
                Catch::TestRegistry::instance().add( \
                    Catch::TestCase(name, __FILE__, __LINE__, &CATCH_INTERNAL_CONCAT(CATCH_INTERNAL_TestFunction, __LINE__)) \
                ); \
            } \
        }; \
        static CATCH_INTERNAL_CONCAT(CATCH_INTERNAL_TestRegistrar, __LINE__) CATCH_INTERNAL_CONCAT(CATCH_INTERNAL_testRegistrar, __LINE__); \
    } \
    static void CATCH_INTERNAL_CONCAT(CATCH_INTERNAL_TestFunction, __LINE__)()

#define REQUIRE(expr) \
    do { \
        if (!(expr)) { \
            std::ostringstream ss; \
            ss << "REQUIRE failed: " << #expr << " at " << __FILE__ << ":" << __LINE__; \
            throw Catch::AssertionException(ss.str()); \
        } \
    } while(0)

#define REQUIRE_FALSE(expr) REQUIRE(!(expr))

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cout << "CHECK failed: " << #expr << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
        } \
    } while(0)

#define SECTION(name) \
    if (bool CATCH_INTERNAL_sectionPassed = true)

#ifdef CATCH_CONFIG_MAIN
int main() {
    return Catch::TestRegistry::instance().run();
}
#endif

#endif // CATCH_HPP_INCLUDED
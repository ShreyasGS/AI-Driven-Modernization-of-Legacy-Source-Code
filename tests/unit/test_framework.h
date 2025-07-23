#ifndef test_framework_h
#define test_framework_h

#include <iostream>
#include <string>
#include <vector>
#include <functional>

// Simple test framework for modernized methods
class TestFramework {
public:
    // Test case structure
    struct TestCase {
        std::string name;
        std::function<bool()> test_function;
        bool success;
        std::string error_message;
    };

    // Add a test case
    void add_test(const std::string& name, std::function<bool()> test_function) {
        TestCase test_case;
        test_case.name = name;
        test_case.test_function = test_function;
        test_case.success = false;
        test_case.error_message = "";
        test_cases.push_back(test_case);
    }

    // Run all test cases
    void run_tests() {
        std::cout << "Running " << test_cases.size() << " tests...\n";
        
        int passed = 0;
        for (auto& test_case : test_cases) {
            std::cout << "  Running test: " << test_case.name << "... ";
            try {
                test_case.success = test_case.test_function();
                if (test_case.success) {
                    std::cout << "PASSED\n";
                    passed++;
                } else {
                    std::cout << "FAILED\n";
                }
            } catch (const std::exception& e) {
                test_case.success = false;
                test_case.error_message = e.what();
                std::cout << "FAILED (exception: " << e.what() << ")\n";
            } catch (...) {
                test_case.success = false;
                test_case.error_message = "Unknown exception";
                std::cout << "FAILED (unknown exception)\n";
            }
        }
        
        std::cout << "\nTest summary: " << passed << " of " << test_cases.size() << " tests passed.\n";
    }

private:
    std::vector<TestCase> test_cases;
};

// Assertion helpers
#define ASSERT(condition) if (!(condition)) { std::cout << "Assertion failed: " #condition " at " __FILE__ ":" << __LINE__ << std::endl; return false; }
#define ASSERT_EQUAL(expected, actual) if ((expected) != (actual)) { std::cout << "Assertion failed: " #expected " == " #actual " at " __FILE__ ":" << __LINE__ << "\n  Expected: " << (expected) << "\n  Actual: " << (actual) << std::endl; return false; }
#define ASSERT_TRUE(condition) ASSERT(condition)
#define ASSERT_FALSE(condition) ASSERT(!(condition))

#endif // test_framework_h 
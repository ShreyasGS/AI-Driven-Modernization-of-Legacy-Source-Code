# Unit Testing Guide for Mozilla 1.0 Modernization

This guide provides a comprehensive approach to unit testing modernized code in the Mozilla 1.0 codebase. It covers testing strategies, frameworks, and best practices to ensure that modernized code maintains the same behavior as the original code.

## Testing Philosophy

Our testing philosophy is based on several key principles:

1. **Verify Correctness**: Ensure that modernized code maintains the same behavior as the original code.
2. **Test Edge Cases**: Test boundary conditions, error cases, and other edge cases.
3. **Maintain Independence**: Tests should be independent of each other and the surrounding codebase.
4. **Simplify Dependencies**: Minimize dependencies on the Mozilla codebase to make tests more maintainable.
5. **Automate Testing**: Make it easy to run tests as part of the development process.

## Testing Approach

We've implemented a multi-layered testing approach:

### 1. Minimal C Tests

Minimal C tests are designed to verify the core concepts of our modernization patterns without dependencies on the Mozilla codebase. These tests:

- Use simple C structs to represent Result<T> and Maybe<T> types
- Mock the behavior of the modernized methods and their compatibility wrappers
- Test various scenarios (success, failure, null inputs, etc.)
- Run quickly and reliably without dependencies

Example of a minimal test for the Maybe<T> pattern:

```c
// Mock implementation of FetchAnchorOffsetModern
IntMaybe FetchAnchorOffsetModern(int hasAnchorNode, int anchorOffset) {
    if (!hasAnchorNode) {
        return Nothing_Int();
    }
    return Some_Int(anchorOffset);
}

// Test FetchAnchorOffsetModern with anchor node
int test_fetch_anchor_offset_with_anchor() {
    // Setup
    int hasAnchorNode = 1;
    int anchorOffset = 42;
    
    // Call the modernized method
    IntMaybe result = FetchAnchorOffsetModern(hasAnchorNode, anchorOffset);
    
    // Verify
    if (!result.hasValue) {
        printf("FAIL: Expected result to have a value\n");
        return 1;
    }
    
    if (result.value != 42) {
        printf("FAIL: Expected value to be 42, got %d\n", result.value);
        return 1;
    }
    
    return 0;
}
```

### 2. Unit Tests

Unit tests are designed to verify that our modernized methods work correctly in the context of the Mozilla codebase. These tests:

- Use mock implementations of Mozilla interfaces
- Test the actual modernized methods
- Verify that the modernized methods maintain the same behavior as the original methods
- Test various scenarios (success, failure, edge cases, etc.)

Example of a unit test for the Result<T> pattern:

```cpp
// Test FetchAnchorParentModern with a valid anchor node and parent
bool test_fetch_anchor_parent_with_valid_parent() {
    // Create test nodes
    MockDOMNode* anchor_node = new MockDOMNode();
    MockDOMNode* parent_node = new MockDOMNode();
    
    // Set up parent-child relationship
    anchor_node->SetParentNode(parent_node);
    
    // Create selection and set up test data
    nsTypedSelection selection;
    selection.SetupTestData(anchor_node, 5, nullptr, 0);
    
    // Call the modernized method
    auto result = mozilla::FetchAnchorParentModern(&selection);
    
    // Check that the result is OK and contains the parent node
    ASSERT_TRUE(result.isOk());
    ASSERT_FALSE(result.isErr());
    
    nsCOMPtr<nsIDOMNode> returned_parent = result.unwrap();
    ASSERT_TRUE(returned_parent != nullptr);
    ASSERT_TRUE(returned_parent.get() == parent_node);
    
    // Clean up (parent_node will be released by the nsCOMPtr)
    anchor_node->Release();
    
    return true;
}
```

### 3. Integration Tests

Integration tests will verify that our modernized methods work correctly in the context of the Mozilla codebase. These tests will:

- Use the actual Mozilla codebase
- Test the modernized methods in the context of the Mozilla codebase
- Verify that the modernized methods maintain the same behavior as the original methods
- Test various scenarios (success, failure, edge cases, etc.)

## Testing Framework

We've created a simple testing framework to make it easy to write and run tests:

### 1. Minimal C Test Framework

The minimal C test framework is designed to be simple and lightweight:

- Uses C macros for assertions
- Provides a simple test runner
- Reports test results in a readable format

Example of the minimal C test framework:

```c
// Simple test framework
#define ASSERT(condition) \
    if (!(condition)) { \
        printf("Assertion failed: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
        return 1; \
    }

#define ASSERT_EQUAL(expected, actual) \
    if ((expected) != (actual)) { \
        printf("Assertion failed: %s == %s at %s:%d\n", #expected, #actual, __FILE__, __LINE__); \
        printf("  Expected: %d\n", (int)(expected)); \
        printf("  Actual: %d\n", (int)(actual)); \
        return 1; \
    }
```

### 2. C++ Test Framework

The C++ test framework is designed to be more feature-rich:

- Uses C++ classes for test organization
- Provides more sophisticated assertions
- Supports test fixtures and setup/teardown
- Reports test results in a more detailed format

Example of the C++ test framework:

```cpp
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
```

## Mock Implementations

To test our modernized methods in isolation, we've created mock implementations of Mozilla interfaces:

### 1. Mock DOM Nodes

Mock DOM nodes implement the nsIDOMNode interface with minimal functionality:

```cpp
// Mock implementation of nsIDOMNode for testing
class MockDOMNode : public nsIDOMNode {
public:
    MockDOMNode() : mRefCnt(0), mParent(nullptr) {}
    
    // nsISupports methods
    NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr) override {
        *aInstancePtr = this;
        return NS_OK;
    }
    
    NS_IMETHOD_(nsrefcnt) AddRef(void) override {
        return ++mRefCnt;
    }
    
    NS_IMETHOD_(nsrefcnt) Release(void) override {
        nsrefcnt count = --mRefCnt;
        if (count == 0) {
            delete this;
        }
        return count;
    }
    
    // nsIDOMNode methods (minimal implementation)
    NS_IMETHOD GetParentNode(nsIDOMNode** aParentNode) override {
        if (mParent) {
            *aParentNode = mParent;
            mParent->AddRef();
            return NS_OK;
        }
        *aParentNode = nullptr;
        return NS_OK;
    }
    
    // Set parent node for testing
    void SetParentNode(nsIDOMNode* aParent) {
        mParent = aParent;
    }
    
private:
    nsrefcnt mRefCnt;
    nsIDOMNode* mParent;
};
```

### 2. Mock Selection

Mock selection implements the nsTypedSelection class with minimal functionality:

```cpp
// Mock implementation of nsTypedSelection for testing
class nsTypedSelection {
public:
    nsTypedSelection() : mAnchorNode(nullptr), mAnchorOffset(0), mFocusNode(nullptr), mFocusOffset(0), mRangeArray(nullptr) {
        // Initialize with default values
    }
    
    ~nsTypedSelection() {
        // Clean up
    }
    
    // Set up test data
    void SetupTestData(nsIDOMNode* aAnchorNode, int32_t aAnchorOffset, nsIDOMNode* aFocusNode, int32_t aFocusOffset) {
        mAnchorNode = aAnchorNode;
        mAnchorOffset = aAnchorOffset;
        mFocusNode = aFocusNode;
        mFocusOffset = aFocusOffset;
    }
    
    // Public members for testing
    nsCOMPtr<nsIDOMNode> mAnchorNode;
    int32_t mAnchorOffset;
    nsCOMPtr<nsIDOMNode> mFocusNode;
    int32_t mFocusOffset;
};
```

## Running Tests

We've created scripts to make it easy to run tests:

### 1. Running Minimal C Tests

To run the minimal C tests:

```bash
cd tests/unit
make -f Makefile.minimal
./minimal_test
```

### 2. Running C++ Unit Tests

To run the C++ unit tests:

```bash
cd tests/unit
make
./test_modernized_selection
```

### 3. Running All Tests

To run all tests:

```bash
cd tests
./run_tests.sh
```

## Writing Tests

When writing tests for modernized code, follow these guidelines:

### 1. Test Both Modernized Methods and Compatibility Wrappers

- Test the modernized methods to ensure they work correctly
- Test the compatibility wrappers to ensure they maintain the same behavior as the original methods

### 2. Test Edge Cases

- Test with null inputs
- Test with invalid inputs
- Test with boundary conditions

### 3. Test Error Handling

- Test that errors are properly propagated
- Test that error codes are correctly converted to Result<T> types
- Test that Result<T> errors are correctly converted back to error codes in compatibility wrappers

### 4. Test Memory Management

- Test that memory is properly managed
- Test that reference counting is correctly handled
- Test that there are no memory leaks

### 5. Use Descriptive Test Names

- Use descriptive names for test functions
- Include the method name and the scenario being tested
- Example: `test_fetch_anchor_parent_with_valid_parent`

## Best Practices

Follow these best practices when testing modernized code:

### 1. Keep Tests Simple

- Focus on testing one thing at a time
- Minimize dependencies on the Mozilla codebase
- Use mock implementations where possible

### 2. Make Tests Repeatable

- Initialize test data in a consistent way
- Clean up after tests
- Avoid dependencies on global state

### 3. Make Tests Fast

- Minimize dependencies on slow operations
- Use mock implementations for external dependencies
- Run tests in parallel where possible

### 4. Make Tests Independent

- Tests should not depend on each other
- Tests should not depend on the order in which they are run
- Tests should not depend on global state

### 5. Make Tests Readable

- Use descriptive names for test functions
- Use comments to explain the purpose of the test
- Use assertions to make the expected behavior clear

## Conclusion

Testing is an essential part of the Mozilla 1.0 modernization effort. By following the guidelines in this document, you can ensure that your modernized code maintains the same behavior as the original code while taking advantage of modern C++ features. 
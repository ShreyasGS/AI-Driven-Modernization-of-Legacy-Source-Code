#include <stdio.h>
#include "mock_nsTypedSelection.h"
#include "modernized_methods.h"

// Simple test framework without standard library dependencies
#define TEST_ASSERT(condition) \
    if (!(condition)) { \
        printf("Assertion failed: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
        return 1; \
    }

#define TEST_ASSERT_EQUAL(expected, actual) \
    if ((expected) != (actual)) { \
        printf("Assertion failed: %s == %s at %s:%d\n", #expected, #actual, __FILE__, __LINE__); \
        printf("  Expected: %d\n", (int)(expected)); \
        printf("  Actual: %d\n", (int)(actual)); \
        return 1; \
    }

// Test FetchAnchorParentModern with a valid anchor node and parent
int test_fetch_anchor_parent_with_valid_parent() {
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
    TEST_ASSERT(result.isOk());
    TEST_ASSERT(!result.isErr());
    
    nsCOMPtr<nsIDOMNode> returned_parent = result.unwrap();
    TEST_ASSERT(returned_parent != nullptr);
    TEST_ASSERT(returned_parent.get() == parent_node);
    
    // Clean up (parent_node will be released by the nsCOMPtr)
    anchor_node->Release();
    
    return 0;
}

// Test FetchAnchorOffsetModern with a valid anchor node
int test_fetch_anchor_offset_with_valid_anchor() {
    // Create test node
    MockDOMNode* anchor_node = new MockDOMNode();
    
    // Create selection and set up test data with offset 42
    nsTypedSelection selection;
    selection.SetupTestData(anchor_node, 42, nullptr, 0);
    
    // Call the modernized method
    auto result = mozilla::FetchAnchorOffsetModern(&selection);
    
    // Check that the result has a value and it's 42
    TEST_ASSERT(result.isSome());
    TEST_ASSERT(!result.isNothing());
    TEST_ASSERT_EQUAL(42, result.value());
    
    // Clean up
    anchor_node->Release();
    
    return 0;
}

// Main function to run all tests
int main() {
    int failures = 0;
    
    printf("Running tests...\n");
    
    printf("  Test: FetchAnchorParentModern with valid parent... ");
    failures += test_fetch_anchor_parent_with_valid_parent();
    printf("%s\n", failures ? "FAILED" : "PASSED");
    
    printf("  Test: FetchAnchorOffsetModern with valid anchor... ");
    int prev_failures = failures;
    failures += test_fetch_anchor_offset_with_valid_anchor();
    printf("%s\n", failures > prev_failures ? "FAILED" : "PASSED");
    
    printf("\nTest summary: %d test(s) failed.\n", failures);
    
    return failures ? 1 : 0;
} 
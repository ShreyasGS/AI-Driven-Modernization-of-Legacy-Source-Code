#include <iostream>
#include "test_framework.h"
#include "mock_nsTypedSelection.h"
#include "modernized_methods.h"

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

// Test FetchAnchorParentModern with a valid anchor node but no parent
bool test_fetch_anchor_parent_with_no_parent() {
    // Create test node with no parent
    MockDOMNode* anchor_node = new MockDOMNode();
    
    // Create selection and set up test data
    nsTypedSelection selection;
    selection.SetupTestData(anchor_node, 5, nullptr, 0);
    
    // Call the modernized method
    auto result = mozilla::FetchAnchorParentModern(&selection);
    
    // Check that the result is OK but the parent is null
    ASSERT_TRUE(result.isOk());
    ASSERT_FALSE(result.isErr());
    ASSERT_TRUE(result.unwrap() == nullptr);
    
    // Clean up
    anchor_node->Release();
    
    return true;
}

// Test FetchAnchorParentModern with no anchor node
bool test_fetch_anchor_parent_with_no_anchor() {
    // Create selection with no anchor node
    nsTypedSelection selection;
    selection.SetupTestData(nullptr, 0, nullptr, 0);
    
    // Call the modernized method
    auto result = mozilla::FetchAnchorParentModern(&selection);
    
    // Check that the result is OK but the parent is null
    ASSERT_TRUE(result.isOk());
    ASSERT_FALSE(result.isErr());
    ASSERT_TRUE(result.unwrap() == nullptr);
    
    return true;
}

// Test FetchAnchorOffsetModern with a valid anchor node
bool test_fetch_anchor_offset_with_valid_anchor() {
    // Create test node
    MockDOMNode* anchor_node = new MockDOMNode();
    
    // Create selection and set up test data with offset 42
    nsTypedSelection selection;
    selection.SetupTestData(anchor_node, 42, nullptr, 0);
    
    // Call the modernized method
    auto result = mozilla::FetchAnchorOffsetModern(&selection);
    
    // Check that the result has a value and it's 42
    ASSERT_TRUE(result.isSome());
    ASSERT_FALSE(result.isNothing());
    ASSERT_EQUAL(42, result.value());
    
    // Clean up
    anchor_node->Release();
    
    return true;
}

// Test FetchAnchorOffsetModern with no anchor node
bool test_fetch_anchor_offset_with_no_anchor() {
    // Create selection with no anchor node
    nsTypedSelection selection;
    selection.SetupTestData(nullptr, 0, nullptr, 0);
    
    // Call the modernized method
    auto result = mozilla::FetchAnchorOffsetModern(&selection);
    
    // Check that the result has no value
    ASSERT_FALSE(result.isSome());
    ASSERT_TRUE(result.isNothing());
    
    return true;
}

// Test compatibility wrapper for FetchAnchorParent
bool test_fetch_anchor_parent_compatibility() {
    // Create test nodes
    MockDOMNode* anchor_node = new MockDOMNode();
    MockDOMNode* parent_node = new MockDOMNode();
    
    // Set up parent-child relationship
    anchor_node->SetParentNode(parent_node);
    
    // Create selection and set up test data
    nsTypedSelection selection;
    selection.SetupTestData(anchor_node, 5, nullptr, 0);
    
    // Call the original method
    nsIDOMNode* parent = nullptr;
    nsresult rv = selection.FetchAnchorParent(&parent);
    
    // Check that the result is OK and parent is correct
    ASSERT_EQUAL(NS_OK, rv);
    ASSERT_TRUE(parent != nullptr);
    ASSERT_TRUE(parent == parent_node);
    
    // Clean up
    parent->Release();
    anchor_node->Release();
    
    return true;
}

// Test compatibility wrapper for FetchAnchorOffset
bool test_fetch_anchor_offset_compatibility() {
    // Create test node
    MockDOMNode* anchor_node = new MockDOMNode();
    
    // Create selection and set up test data with offset 42
    nsTypedSelection selection;
    selection.SetupTestData(anchor_node, 42, nullptr, 0);
    
    // Call the original method
    int32_t offset = 0;
    nsresult rv = selection.FetchAnchorOffset(&offset);
    
    // Check that the result is OK and offset is correct
    ASSERT_EQUAL(NS_OK, rv);
    ASSERT_EQUAL(42, offset);
    
    // Clean up
    anchor_node->Release();
    
    return true;
}

int main() {
    TestFramework framework;
    
    // Add tests for FetchAnchorParentModern
    framework.add_test("FetchAnchorParentModern with valid parent", test_fetch_anchor_parent_with_valid_parent);
    framework.add_test("FetchAnchorParentModern with no parent", test_fetch_anchor_parent_with_no_parent);
    framework.add_test("FetchAnchorParentModern with no anchor", test_fetch_anchor_parent_with_no_anchor);
    
    // Add tests for FetchAnchorOffsetModern
    framework.add_test("FetchAnchorOffsetModern with valid anchor", test_fetch_anchor_offset_with_valid_anchor);
    framework.add_test("FetchAnchorOffsetModern with no anchor", test_fetch_anchor_offset_with_no_anchor);
    
    // Add tests for compatibility wrappers
    framework.add_test("FetchAnchorParent compatibility wrapper", test_fetch_anchor_parent_compatibility);
    framework.add_test("FetchAnchorOffset compatibility wrapper", test_fetch_anchor_offset_compatibility);
    
    // Run all tests
    framework.run_tests();
    
    return 0;
} 
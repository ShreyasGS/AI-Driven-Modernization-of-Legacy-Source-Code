# Unit Tests for Mozilla 1.0 Modernization

This directory contains unit tests for the modernized methods in the Mozilla 1.0 codebase.

## Overview

The tests are designed to verify that the modernized methods work correctly and maintain backward compatibility with the original API. The tests use a simple test framework and mock implementations of the Mozilla interfaces.

## Test Structure

- `test_framework.h`: A simple test framework for running unit tests
- `mock_nsTypedSelection.h/cpp`: Mock implementation of nsTypedSelection for testing
- `modernized_methods.h/cpp`: The modernized methods being tested
- `test_modernized_selection.cpp`: The actual test cases

## Running the Tests

To build and run the tests:

```
cd tests/unit
make run
```

This will compile the tests and run them, displaying the results.

## Test Coverage

Currently, the following modernized methods are tested:

1. `FetchAnchorParentModern`: Tests that it correctly retrieves the parent node of the anchor node
   - Test with a valid anchor node and parent
   - Test with a valid anchor node but no parent
   - Test with no anchor node

2. `FetchAnchorOffsetModern`: Tests that it correctly retrieves the offset of the anchor node
   - Test with a valid anchor node
   - Test with no anchor node

3. Compatibility wrappers: Tests that the original API still works correctly
   - Test `FetchAnchorParent` compatibility wrapper
   - Test `FetchAnchorOffset` compatibility wrapper

## Adding New Tests

To add tests for additional modernized methods:

1. Add the method declarations to `modernized_methods.h`
2. Implement the methods in `modernized_methods.cpp`
3. Add test cases to `test_modernized_selection.cpp`
4. Update the `mock_nsTypedSelection.h/cpp` if needed

## Future Improvements

- Add more comprehensive tests for edge cases
- Add performance tests to compare modernized methods with original methods
- Add memory usage tests to verify that modernized methods don't leak memory 
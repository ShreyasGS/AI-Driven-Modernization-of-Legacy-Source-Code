#!/bin/bash

# Script to run all tests for the Mozilla 1.0 modernization project

# Set up error handling
set -e
set -o pipefail

# Print a header
echo "========================================"
echo "Mozilla 1.0 Modernization Tests"
echo "========================================"
echo

# Run minimal tests
echo "Running minimal tests..."
cd unit
make -f Makefile.minimal clean
make -f Makefile.minimal
./minimal_test

# Return to the original directory
cd ..

echo
echo "All tests completed successfully!" 
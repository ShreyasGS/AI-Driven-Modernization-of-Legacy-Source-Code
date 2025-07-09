#!/bin/bash

# Script to index the Mozilla 1.0 codebase and gather statistics
# Creates various reports in the analysis directory

echo "Starting Mozilla 1.0 codebase indexing..."
ANALYSIS_DIR="$(pwd)/analysis"
mkdir -p "$ANALYSIS_DIR/reports"

# 1. Generate file inventory by type
echo "Generating file inventory by type..."
find . -type f -name "*.cpp" | sort > "$ANALYSIS_DIR/reports/cpp_files.txt"
find . -type f -name "*.h" | sort > "$ANALYSIS_DIR/reports/h_files.txt"
find . -type f -name "*.c" | sort > "$ANALYSIS_DIR/reports/c_files.txt"
find . -type f -name "*.idl" | sort > "$ANALYSIS_DIR/reports/idl_files.txt"

# 2. Count files by type
echo "Counting files by type..."
echo "C++ files: $(wc -l < "$ANALYSIS_DIR/reports/cpp_files.txt")" > "$ANALYSIS_DIR/reports/file_counts.txt"
echo "Header files: $(wc -l < "$ANALYSIS_DIR/reports/h_files.txt")" >> "$ANALYSIS_DIR/reports/file_counts.txt"
echo "C files: $(wc -l < "$ANALYSIS_DIR/reports/c_files.txt")" >> "$ANALYSIS_DIR/reports/file_counts.txt"
echo "IDL files: $(wc -l < "$ANALYSIS_DIR/reports/idl_files.txt")" >> "$ANALYSIS_DIR/reports/file_counts.txt"

# 3. Generate directory structure statistics
echo "Analyzing directory structure..."
find . -type d | sort | grep -v "CVS" > "$ANALYSIS_DIR/reports/directories.txt"
echo "Total directories: $(wc -l < "$ANALYSIS_DIR/reports/directories.txt")" >> "$ANALYSIS_DIR/reports/file_counts.txt"

# 4. Count lines of code by type
echo "Counting lines of code by type..."
if [ -s "$ANALYSIS_DIR/reports/cpp_files.txt" ]; then
  xargs wc -l < "$ANALYSIS_DIR/reports/cpp_files.txt" > "$ANALYSIS_DIR/reports/cpp_loc.txt"
fi
if [ -s "$ANALYSIS_DIR/reports/h_files.txt" ]; then
  xargs wc -l < "$ANALYSIS_DIR/reports/h_files.txt" > "$ANALYSIS_DIR/reports/h_loc.txt"
fi
if [ -s "$ANALYSIS_DIR/reports/c_files.txt" ]; then
  xargs wc -l < "$ANALYSIS_DIR/reports/c_files.txt" > "$ANALYSIS_DIR/reports/c_loc.txt"
fi

# 5. Analyze comment density
echo "Analyzing comment density..."
echo "Files with block comments: $(find . -type f -name "*.cpp" -o -name "*.c" -o -name "*.h" | xargs grep -l "/\*" | wc -l)" > "$ANALYSIS_DIR/reports/comment_stats.txt"
echo "Files with line comments: $(find . -type f -name "*.cpp" -o -name "*.c" -o -name "*.h" | xargs grep -l "//" | wc -l)" >> "$ANALYSIS_DIR/reports/comment_stats.txt"

# 6. Find largest files (potential complexity hotspots)
echo "Finding largest files (potential complexity hotspots)..."
find . -type f -name "*.cpp" -o -name "*.c" | xargs wc -l | sort -nr | head -n 50 > "$ANALYSIS_DIR/reports/largest_files.txt"

# 7. Generate module-level statistics
echo "Generating module-level statistics..."
for dir in xpcom layout content dom js netwerk gfx widget; do
  if [ -d "$dir" ]; then
    echo "Module: $dir" > "$ANALYSIS_DIR/reports/${dir}_stats.txt"
    echo "  C++ files: $(find $dir -name "*.cpp" | wc -l)" >> "$ANALYSIS_DIR/reports/${dir}_stats.txt"
    echo "  Header files: $(find $dir -name "*.h" | wc -l)" >> "$ANALYSIS_DIR/reports/${dir}_stats.txt"
    echo "  C files: $(find $dir -name "*.c" | wc -l)" >> "$ANALYSIS_DIR/reports/${dir}_stats.txt"
    echo "  IDL files: $(find $dir -name "*.idl" | wc -l)" >> "$ANALYSIS_DIR/reports/${dir}_stats.txt"
    echo "  Total lines of code: $(find $dir -name "*.cpp" -o -name "*.c" -o -name "*.h" | xargs wc -l | tail -1)" >> "$ANALYSIS_DIR/reports/${dir}_stats.txt"
  fi
done

# 8. Look for memory management patterns
echo "Analyzing memory management patterns..."
echo "Manual memory allocations:" > "$ANALYSIS_DIR/reports/memory_management.txt"
echo "  malloc calls: $(find . -name "*.cpp" -o -name "*.c" | xargs grep -l "malloc" | wc -l)" >> "$ANALYSIS_DIR/reports/memory_management.txt"
echo "  free calls: $(find . -name "*.cpp" -o -name "*.c" | xargs grep -l "free" | wc -l)" >> "$ANALYSIS_DIR/reports/memory_management.txt"
echo "  new operator: $(find . -name "*.cpp" | xargs grep -l "new " | wc -l)" >> "$ANALYSIS_DIR/reports/memory_management.txt"
echo "  delete operator: $(find . -name "*.cpp" | xargs grep -l "delete " | wc -l)" >> "$ANALYSIS_DIR/reports/memory_management.txt"

# 9. Look for error handling patterns
echo "Analyzing error handling patterns..."
echo "Error handling patterns:" > "$ANALYSIS_DIR/reports/error_handling.txt"
echo "  Files with NS_SUCCEEDED: $(find . -name "*.cpp" -o -name "*.c" | xargs grep -l "NS_SUCCEEDED" | wc -l)" >> "$ANALYSIS_DIR/reports/error_handling.txt"
echo "  Files with NS_FAILED: $(find . -name "*.cpp" -o -name "*.c" | xargs grep -l "NS_FAILED" | wc -l)" >> "$ANALYSIS_DIR/reports/error_handling.txt"
echo "  Files with try/catch: $(find . -name "*.cpp" | xargs grep -l "try" | xargs grep -l "catch" | wc -l)" >> "$ANALYSIS_DIR/reports/error_handling.txt"

# 10. Generate summary report
echo "Generating summary report..."
cat "$ANALYSIS_DIR/reports/file_counts.txt" > "$ANALYSIS_DIR/codebase_summary.txt"
echo "" >> "$ANALYSIS_DIR/codebase_summary.txt"
echo "Comment Statistics:" >> "$ANALYSIS_DIR/codebase_summary.txt"
cat "$ANALYSIS_DIR/reports/comment_stats.txt" >> "$ANALYSIS_DIR/codebase_summary.txt"
echo "" >> "$ANALYSIS_DIR/codebase_summary.txt"
echo "Memory Management:" >> "$ANALYSIS_DIR/codebase_summary.txt"
cat "$ANALYSIS_DIR/reports/memory_management.txt" >> "$ANALYSIS_DIR/codebase_summary.txt"
echo "" >> "$ANALYSIS_DIR/codebase_summary.txt"
echo "Error Handling:" >> "$ANALYSIS_DIR/codebase_summary.txt"
cat "$ANALYSIS_DIR/reports/error_handling.txt" >> "$ANALYSIS_DIR/codebase_summary.txt"

echo "Codebase indexing complete. Results available in $ANALYSIS_DIR/codebase_summary.txt" 
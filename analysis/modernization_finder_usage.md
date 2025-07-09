# Mozilla 1.0 Modernization Finder Tool

## Overview

The Modernization Finder Tool is a Python script designed to identify patterns in the Mozilla 1.0 codebase that could benefit from modernization. It analyzes C/C++ code files, detects various patterns (like manual memory management, error handling, type casting, etc.), and generates comprehensive reports.

## Features

- Searches for multiple modernization patterns in C/C++ code
- Filters by component or directory
- Generates both Markdown and JSON reports
- Provides statistics by component and pattern
- Identifies top files with modernization opportunities

## Detected Patterns

The tool detects the following patterns:

1. **Memory Management**
   - Manual memory allocation/deallocation (malloc/free)
   - Raw new/delete operators
   - Array allocation/deallocation

2. **Error Handling**
   - Error code returns (NS_OK/NS_ERROR)
   - Error code checks (NS_FAILED/NS_SUCCEEDED)

3. **Type Casting**
   - C-style casts

4. **Parameter Patterns**
   - Out parameters (double pointers)
   - Multiple out parameters

5. **Reference Counting**
   - Manual AddRef/Release calls

6. **QueryInterface Patterns**
   - Manual QueryInterface calls

7. **Null Checking**
   - Null pointer checks

8. **Modern C++ Opportunities**
   - Empty constructors/destructors that could use `= default`

## Usage

### Basic Usage

```bash
./analysis/modernization_finder.py
```

This will scan the entire codebase with default settings and generate reports in the `analysis/reports` directory.

### Command Line Options

```bash
./analysis/modernization_finder.py --dirs <directories> --include <patterns> --exclude <patterns> --output-dir <directory> --component <name> --top <number>
```

- `--dirs`: Directories to search (default: current directory)
- `--include`: File patterns to include (default: *.cpp, *.h, *.c)
- `--exclude`: Directories or files to exclude (default: CVS, .git)
- `--output-dir`: Output directory for reports (default: analysis/reports)
- `--component`: Specific component to analyze (e.g., "dom", "layout")
- `--top`: Number of top files to show in the report (default: 10)

### Examples

Analyze only the DOM component:
```bash
./analysis/modernization_finder.py --component dom
```

Analyze specific directories:
```bash
./analysis/modernization_finder.py --dirs dom/src layout/html
```

Exclude test files:
```bash
./analysis/modernization_finder.py --exclude CVS .git tests
```

## Output

The tool generates two types of reports:

1. **Markdown Report** (`modernization_opportunities.md`):
   - Summary of findings
   - Overall statistics by pattern
   - Breakdown by component
   - Top files with modernization opportunities

2. **JSON Report** (`modernization_opportunities.json`):
   - Machine-readable format for further processing
   - Contains all the data from the Markdown report

## Integration with Modernization Process

The Modernization Finder Tool supports our modernization process by:

1. **Identifying Opportunities**: Finding code patterns that could benefit from modernization
2. **Measuring Progress**: Tracking the reduction of legacy patterns over time
3. **Prioritizing Efforts**: Identifying components and files with the most modernization opportunities
4. **Validating Implementations**: Verifying that modernized components follow best practices

## Next Steps

1. Run the tool on the entire codebase to establish a baseline
2. Integrate the tool into our CI/CD pipeline to track progress
3. Develop additional patterns to detect more modernization opportunities
4. Create automated refactoring tools based on the patterns detected 
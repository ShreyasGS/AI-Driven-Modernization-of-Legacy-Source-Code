#!/usr/bin/env python3

"""
Mozilla 1.0 Codebase Modernization Finder
This script identifies patterns in C/C++ code that could benefit from modernization.
"""

import os
import re
import sys
import argparse
import json
from collections import defaultdict, Counter
from datetime import datetime

# Configuration
DEFAULT_OUTPUT_DIR = os.path.join(os.getcwd(), "analysis", "reports")

# Patterns to look for
PATTERNS = {
    # Memory management patterns
    "manual_allocation": {
        "regex": r'\b(malloc|calloc|realloc)\s*\(',
        "description": "Manual memory allocation",
        "modernization": "Replace with smart pointers or RAII pattern"
    },
    "manual_deallocation": {
        "regex": r'\bfree\s*\(',
        "description": "Manual memory deallocation",
        "modernization": "Replace with smart pointers or RAII pattern"
    },
    "raw_new": {
        "regex": r'\bnew\s+\w+',
        "description": "Raw new operator",
        "modernization": "Use smart pointers (std::unique_ptr, RefPtr)"
    },
    "raw_delete": {
        "regex": r'\bdelete\s+\w+',
        "description": "Raw delete operator",
        "modernization": "Use smart pointers (std::unique_ptr, RefPtr)"
    },
    "array_new": {
        "regex": r'\bnew\s+\w+\s*\[',
        "description": "Raw array allocation",
        "modernization": "Use std::vector or other container"
    },
    "array_delete": {
        "regex": r'\bdelete\s*\[\s*\]',
        "description": "Raw array deallocation",
        "modernization": "Use std::vector or other container"
    },
    
    # Error handling patterns
    "error_code_return": {
        "regex": r'\breturn\s+NS_(OK|ERROR)',
        "description": "Error code return",
        "modernization": "Use result types or exceptions"
    },
    "error_check": {
        "regex": r'(NS_FAILED|NS_SUCCEEDED)\s*\(',
        "description": "Error code check",
        "modernization": "Use result types with explicit success/failure"
    },
    
    # Type casting patterns
    "c_style_cast": {
        "regex": r'\(\s*(const\s+)?\w+(\s*\*+\s*)+\)\s*\w+',
        "description": "C-style cast",
        "modernization": "Use static_cast, dynamic_cast, or const_cast"
    },
    
    # Out parameter patterns
    "out_parameter": {
        "regex": r'\w+\s*\(\s*[^)]*\w+\s*\*\*\s*\w+\s*[,)]',
        "description": "Out parameter",
        "modernization": "Return value or result type"
    },
    
    # Reference counting patterns
    "addref_call": {
        "regex": r'(NS_ADDREF|AddRef)\s*\(',
        "description": "Manual AddRef call",
        "modernization": "Use RefPtr or nsCOMPtr"
    },
    "release_call": {
        "regex": r'(NS_RELEASE|Release)\s*\(',
        "description": "Manual Release call",
        "modernization": "Use RefPtr or nsCOMPtr"
    },
    
    # QueryInterface patterns
    "query_interface": {
        "regex": r'QueryInterface\s*\(',
        "description": "Manual QueryInterface call",
        "modernization": "Use do_QueryInterface or as<T>()"
    },
    
    # Null checking patterns
    "null_check": {
        "regex": r'(if\s*\(\s*(\!|null\s*\!=)\s*\w+\s*\)|if\s*\(\s*\w+\s*\=\=\s*null\s*\))',
        "description": "Null pointer check",
        "modernization": "Use std::optional or result type"
    },
    
    # Modern C++ opportunities
    "default_opportunity": {
        "regex": r'\w+::\~?\w+\s*\(\s*\)\s*\{\s*\}',
        "description": "Empty constructor/destructor",
        "modernization": "Use = default"
    },
    "multiple_out_params": {
        "regex": r'\w+\s*\([^)]*\w+\s*\&[^,)]*,[^)]*\w+\s*\&',
        "description": "Multiple out parameters",
        "modernization": "Return std::tuple or custom struct"
    }
}

def parse_arguments():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(description='Find modernization opportunities in C/C++ code')
    parser.add_argument('--dirs', nargs='+', default=['.'],
                        help='Directories to search (default: current directory)')
    parser.add_argument('--include', nargs='+', default=['*.cpp', '*.h', '*.c'],
                        help='File patterns to include (default: *.cpp, *.h, *.c)')
    parser.add_argument('--exclude', nargs='+', default=['CVS', '.git'],
                        help='Directories or files to exclude (default: CVS, .git)')
    parser.add_argument('--output-dir', default=DEFAULT_OUTPUT_DIR,
                        help=f'Output directory for reports (default: {DEFAULT_OUTPUT_DIR})')
    parser.add_argument('--component', default=None,
                        help='Specific component to analyze (e.g., "dom", "layout")')
    parser.add_argument('--top', type=int, default=10,
                        help='Number of top files to show in the report (default: 10)')
    return parser.parse_args()

def should_include_file(filename, include_patterns, exclude_patterns):
    """Check if a file should be included based on patterns"""
    # Check exclude patterns first
    for pattern in exclude_patterns:
        if pattern in filename:
            return False
    
    # Then check include patterns
    for pattern in include_patterns:
        if pattern.startswith('*.'):
            if filename.endswith(pattern[1:]):
                return True
        elif pattern in filename:
            return True
    
    return False

def find_files(directories, include_patterns, exclude_patterns):
    """Find files matching the include patterns and not matching exclude patterns"""
    files = []
    for directory in directories:
        for root, dirs, filenames in os.walk(directory):
            # Skip excluded directories
            dirs[:] = [d for d in dirs if d not in exclude_patterns]
            
            for filename in filenames:
                filepath = os.path.join(root, filename)
                if should_include_file(filepath, include_patterns, exclude_patterns):
                    files.append(filepath)
    
    return files

def analyze_file(filepath, patterns):
    """Analyze a file for modernization opportunities"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        results = {}
        for pattern_name, pattern_info in patterns.items():
            matches = re.findall(pattern_info["regex"], content)
            if matches:
                results[pattern_name] = len(matches)
        
        return results
    except Exception as e:
        print(f"Error analyzing {filepath}: {e}")
        return {}

def get_component_name(filepath):
    """Extract component name from filepath"""
    parts = filepath.split(os.path.sep)
    if len(parts) > 1:
        return parts[0]
    return "unknown"

def analyze_files(files, patterns, component_filter=None):
    """Analyze files for modernization opportunities"""
    results = []
    
    total_files = len(files)
    print(f"Analyzing {total_files} files...")
    
    for i, filepath in enumerate(files):
        if i % 100 == 0:
            print(f"Progress: {i}/{total_files} files ({i/total_files*100:.1f}%)")
        
        component = get_component_name(filepath)
        if component_filter and component != component_filter:
            continue
        
        file_results = analyze_file(filepath, patterns)
        if file_results:
            results.append({
                "filepath": filepath,
                "component": component,
                "patterns": file_results
            })
    
    return results

def generate_report(results, output_dir, top_n=10):
    """Generate a report of modernization opportunities"""
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # Aggregate results by component
    component_results = defaultdict(lambda: defaultdict(int))
    file_counts = Counter()
    pattern_by_file = defaultdict(list)
    
    for result in results:
        component = result["component"]
        file_counts[component] += 1
        
        for pattern_name, count in result["patterns"].items():
            component_results[component][pattern_name] += count
            pattern_by_file[result["filepath"]].append((pattern_name, count))
    
    # Generate timestamp
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    # Create markdown report
    report_path = os.path.join(output_dir, "modernization_opportunities.md")
    with open(report_path, 'w') as f:
        f.write(f"# Mozilla 1.0 Codebase Modernization Opportunities\n\n")
        f.write(f"Report generated on: {timestamp}\n\n")
        f.write(f"## Summary\n\n")
        f.write(f"Analyzed {len(results)} files with modernization opportunities across {len(component_results)} components.\n\n")
        
        # Overall statistics
        f.write(f"## Overall Statistics\n\n")
        
        # Calculate total occurrences by pattern
        total_by_pattern = defaultdict(int)
        for component, patterns in component_results.items():
            for pattern_name, count in patterns.items():
                total_by_pattern[pattern_name] += count
        
        # Sort patterns by total occurrences
        sorted_patterns = sorted(total_by_pattern.items(), key=lambda x: x[1], reverse=True)
        
        f.write("| Pattern | Occurrences | Description | Modernization Approach |\n")
        f.write("|---------|-------------|-------------|------------------------|\n")
        
        for pattern_name, count in sorted_patterns:
            pattern_info = PATTERNS[pattern_name]
            f.write(f"| {pattern_name} | {count} | {pattern_info['description']} | {pattern_info['modernization']} |\n")
        
        # Component breakdown
        f.write(f"\n## Component Breakdown\n\n")
        
        # Sort components by total occurrences
        component_totals = {comp: sum(patterns.values()) for comp, patterns in component_results.items()}
        sorted_components = sorted(component_totals.items(), key=lambda x: x[1], reverse=True)
        
        for component, total in sorted_components:
            patterns = component_results[component]
            f.write(f"\n### {component} ({file_counts[component]} files)\n\n")
            
            f.write("| Pattern | Occurrences | Description | Modernization Approach |\n")
            f.write("|---------|-------------|-------------|------------------------|\n")
            
            # Sort patterns by occurrences
            sorted_patterns = sorted(patterns.items(), key=lambda x: x[1], reverse=True)
            
            for pattern_name, count in sorted_patterns:
                pattern_info = PATTERNS[pattern_name]
                f.write(f"| {pattern_name} | {count} | {pattern_info['description']} | {pattern_info['modernization']} |\n")
        
        # Top files with most modernization opportunities
        f.write(f"\n## Top {top_n} Files with Most Modernization Opportunities\n\n")
        
        # Calculate total opportunities per file
        file_totals = {filepath: sum(count for _, count in patterns) for filepath, patterns in pattern_by_file.items()}
        top_files = sorted(file_totals.items(), key=lambda x: x[1], reverse=True)[:top_n]
        
        f.write("| File | Component | Total Opportunities | Top Patterns |\n")
        f.write("|------|-----------|---------------------|-------------|\n")
        
        for filepath, total in top_files:
            component = get_component_name(filepath)
            patterns = pattern_by_file[filepath]
            sorted_patterns = sorted(patterns, key=lambda x: x[1], reverse=True)
            top_patterns = ", ".join([f"{pattern} ({count})" for pattern, count in sorted_patterns[:3]])
            f.write(f"| {filepath} | {component} | {total} | {top_patterns} |\n")
    
    # Create JSON report for programmatic use
    json_path = os.path.join(output_dir, "modernization_opportunities.json")
    with open(json_path, 'w') as f:
        json.dump({
            "timestamp": timestamp,
            "summary": {
                "total_files": len(results),
                "total_components": len(component_results)
            },
            "patterns": {name: {"count": count, **PATTERNS[name]} 
                        for name, count in total_by_pattern.items()},
            "components": {comp: {"file_count": file_counts[comp], "patterns": patterns} 
                          for comp, patterns in component_results.items()},
            "top_files": [{"filepath": filepath, "total": total, 
                          "component": get_component_name(filepath),
                          "patterns": pattern_by_file[filepath]} 
                         for filepath, total in top_files]
        }, f, indent=2)
    
    print(f"Report written to {report_path}")
    print(f"JSON data written to {json_path}")
    
    return report_path, json_path

def main():
    """Main function"""
    args = parse_arguments()
    
    # Create output directory if it doesn't exist
    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)
    
    # Find files to analyze
    print(f"Searching for files in {', '.join(args.dirs)}...")
    files = find_files(args.dirs, args.include, args.exclude)
    print(f"Found {len(files)} files to analyze")
    
    # Analyze files
    results = analyze_files(files, PATTERNS, args.component)
    print(f"Found modernization opportunities in {len(results)} files")
    
    # Generate report
    report_path, json_path = generate_report(results, args.output_dir, args.top)
    
    print(f"Analysis complete! Reports available at:")
    print(f"  - {report_path}")
    print(f"  - {json_path}")

if __name__ == "__main__":
    main() 
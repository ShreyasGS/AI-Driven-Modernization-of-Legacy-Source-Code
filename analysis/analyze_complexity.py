#!/usr/bin/env python3

"""
Mozilla 1.0 Codebase Analysis Script
This script analyzes C/C++ files to identify modernization opportunities
and complexity issues.
"""

import os
import re
import sys
import subprocess
from collections import defaultdict, Counter

# Configuration
ANALYSIS_DIR = os.path.join(os.getcwd(), "analysis")
REPORTS_DIR = os.path.join(ANALYSIS_DIR, "reports")
SAMPLE_SIZE = 100  # Number of files to analyze in detail

# Patterns to look for
PATTERNS = {
    # Memory management patterns
    "manual_alloc": re.compile(r'\b(malloc|calloc|realloc)\s*\('),
    "manual_free": re.compile(r'\bfree\s*\('),
    "new_op": re.compile(r'\bnew\s+\w+'),
    "delete_op": re.compile(r'\bdelete\s+\w+'),
    "array_new": re.compile(r'\bnew\s+\w+\s*\['),
    "array_delete": re.compile(r'\bdelete\s*\[\s*\]'),
    
    # Error handling patterns
    "ns_result": re.compile(r'\bnsresult\b', re.IGNORECASE),
    "ns_succeeded": re.compile(r'\bNS_SUCCEEDED\s*\('),
    "ns_failed": re.compile(r'\bNS_FAILED\s*\('),
    "try_catch": re.compile(r'\btry\s*{'),
    
    # Resource management
    "file_open": re.compile(r'\b(fopen|open|PR_Open)\s*\('),
    "file_close": re.compile(r'\b(fclose|close|PR_Close)\s*\('),
    
    # Complex code patterns
    "goto_statement": re.compile(r'\bgoto\s+\w+;'),
    "nested_if": re.compile(r'if\s*\([^{]*\)\s*{[^}]*if\s*\('),
    "long_function": re.compile(r'(\w+)::\1\s*\([^)]*\)\s*{|\w+\s+\w+\s*\([^)]*\)\s*{'),
    
    # Potential modernization targets
    "c_style_cast": re.compile(r'\(\s*(const\s+)?\w+(\s*\*+\s*)+\)\s*\w+'),
    "raw_pointer": re.compile(r'\b\w+\s*\*\s*\w+\s*='),
}

def count_patterns_in_file(filepath):
    """Count occurrences of patterns in a file"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
            
        results = {}
        for pattern_name, pattern in PATTERNS.items():
            matches = pattern.findall(content)
            results[pattern_name] = len(matches)
            
        # Count lines and estimate complexity
        lines = content.count('\n')
        functions = content.count('{') - content.count('}')  # Rough estimate
        complexity = estimate_complexity(content)
            
        return {
            'patterns': results,
            'lines': lines,
            'estimated_functions': max(0, functions),
            'complexity': complexity
        }
    except Exception as e:
        print(f"Error analyzing {filepath}: {e}")
        return None

def estimate_complexity(content):
    """Estimate code complexity based on various factors"""
    # This is a simplified complexity estimation
    score = 0
    
    # Control flow complexity
    score += content.count('if (') * 1
    score += content.count('else if') * 1.5
    score += content.count('switch') * 2
    score += content.count('case ') * 0.5
    score += content.count('for (') * 2
    score += content.count('while (') * 2
    score += content.count('goto ') * 3
    
    # Nesting complexity - crude approximation
    open_braces = 0
    max_nesting = 0
    for char in content:
        if char == '{':
            open_braces += 1
            max_nesting = max(max_nesting, open_braces)
        elif char == '}':
            open_braces = max(0, open_braces - 1)
    
    score += max_nesting * 3
    
    return score

def analyze_sample_files():
    """Analyze a sample of files for detailed metrics"""
    print("Analyzing sample files for detailed metrics...")
    
    # Get list of C/C++ files
    cpp_files = []
    with open(os.path.join(REPORTS_DIR, "cpp_files.txt"), 'r') as f:
        cpp_files.extend(f.read().splitlines())
    
    c_files = []
    with open(os.path.join(REPORTS_DIR, "c_files.txt"), 'r') as f:
        c_files.extend(f.read().splitlines())
    
    h_files = []
    with open(os.path.join(REPORTS_DIR, "h_files.txt"), 'r') as f:
        h_files.extend(f.read().splitlines())
    
    # Select sample files from each category
    import random
    random.seed(42)  # For reproducibility
    
    sample_cpp = random.sample(cpp_files, min(SAMPLE_SIZE // 2, len(cpp_files)))
    sample_c = random.sample(c_files, min(SAMPLE_SIZE // 4, len(c_files)))
    sample_h = random.sample(h_files, min(SAMPLE_SIZE // 4, len(h_files)))
    
    sample_files = sample_cpp + sample_c + sample_h
    
    # Analyze each sample file
    results = []
    for filepath in sample_files:
        print(f"  Analyzing {filepath}...")
        analysis = count_patterns_in_file(filepath)
        if analysis:
            analysis['filepath'] = filepath
            results.append(analysis)
    
    return results

def generate_modernization_report(sample_results):
    """Generate a report on modernization opportunities"""
    print("Generating modernization opportunities report...")
    
    # Aggregate metrics
    total_files = len(sample_results)
    pattern_totals = defaultdict(int)
    complexity_scores = []
    lines_of_code = []
    
    for result in sample_results:
        patterns = result.get('patterns', {})
        for pattern, count in patterns.items():
            pattern_totals[pattern] += count
        
        complexity_scores.append(result.get('complexity', 0))
        lines_of_code.append(result.get('lines', 0))
    
    # Calculate averages
    avg_complexity = sum(complexity_scores) / len(complexity_scores) if complexity_scores else 0
    avg_loc = sum(lines_of_code) / len(lines_of_code) if lines_of_code else 0
    
    # Generate report
    report = []
    report.append("# Mozilla 1.0 Codebase Modernization Opportunities")
    report.append(f"\nAnalysis based on {total_files} sample files\n")
    
    report.append("## Code Complexity")
    report.append(f"- Average complexity score: {avg_complexity:.2f}")
    report.append(f"- Average lines per file: {avg_loc:.2f}")
    report.append(f"- Files with high complexity (>100): {sum(1 for s in complexity_scores if s > 100)}")
    
    report.append("\n## Memory Management")
    report.append(f"- Manual allocation calls: {pattern_totals['manual_alloc']}")
    report.append(f"- Manual free calls: {pattern_totals['manual_free']}")
    report.append(f"- New operator usage: {pattern_totals['new_op']}")
    report.append(f"- Delete operator usage: {pattern_totals['delete_op']}")
    report.append(f"- Array new usage: {pattern_totals['array_new']}")
    report.append(f"- Array delete usage: {pattern_totals['array_delete']}")
    
    report.append("\n## Error Handling")
    report.append(f"- NS_SUCCEEDED usage: {pattern_totals['ns_succeeded']}")
    report.append(f"- NS_FAILED usage: {pattern_totals['ns_failed']}")
    report.append(f"- Try/catch blocks: {pattern_totals['try_catch']}")
    
    report.append("\n## Resource Management")
    report.append(f"- File open calls: {pattern_totals['file_open']}")
    report.append(f"- File close calls: {pattern_totals['file_close']}")
    
    report.append("\n## Code Quality Issues")
    report.append(f"- Goto statements: {pattern_totals['goto_statement']}")
    report.append(f"- Nested if statements: {pattern_totals['nested_if']}")
    report.append(f"- C-style casts: {pattern_totals['c_style_cast']}")
    report.append(f"- Raw pointer assignments: {pattern_totals['raw_pointer']}")
    
    report.append("\n## Top Modernization Opportunities")
    
    # Calculate opportunities
    opportunities = [
        ("Replace manual memory management with RAII", 
         pattern_totals['manual_alloc'] + pattern_totals['manual_free']),
        ("Replace raw pointers with smart pointers", 
         pattern_totals['raw_pointer']),
        ("Replace C-style casts with C++ casts", 
         pattern_totals['c_style_cast']),
        ("Improve error handling (replace error codes with exceptions where appropriate)", 
         pattern_totals['ns_succeeded'] + pattern_totals['ns_failed']),
        ("Refactor complex functions", 
         sum(1 for s in complexity_scores if s > 50))
    ]
    
    # Sort by impact
    opportunities.sort(key=lambda x: x[1], reverse=True)
    
    for i, (opportunity, count) in enumerate(opportunities, 1):
        report.append(f"{i}. {opportunity} ({count} instances)")
    
    # Write report to file
    with open(os.path.join(ANALYSIS_DIR, "modernization_opportunities.md"), 'w') as f:
        f.write('\n'.join(report))
    
    print(f"Report written to {os.path.join(ANALYSIS_DIR, 'modernization_opportunities.md')}")

def main():
    """Main function"""
    print("Starting detailed code analysis...")
    
    # Create directories if they don't exist
    os.makedirs(REPORTS_DIR, exist_ok=True)
    
    # Analyze sample files
    sample_results = analyze_sample_files()
    
    # Generate modernization report
    generate_modernization_report(sample_results)
    
    print("Analysis complete!")

if __name__ == "__main__":
    main() 
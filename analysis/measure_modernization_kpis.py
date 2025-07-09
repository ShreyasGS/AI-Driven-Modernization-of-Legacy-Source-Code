#!/usr/bin/env python3
"""
Modernization KPI Measurement Script

This script analyzes C/C++ code to measure various KPIs related to the Mozilla 1.0 modernization effort.
"""

import os
import re
import sys
import argparse
import json
from collections import defaultdict
import subprocess
from datetime import datetime

# Patterns to look for in the code
PATTERNS = {
    'manual_reference_counting': {
        'pattern': r'\b(AddRef|Release)\b(?!\s*\=)',
        'description': 'Manual Reference Counting'
    },
    'error_code_returns': {
        'pattern': r'\breturn\s+NS_',
        'description': 'Error Code Returns'
    },
    'c_style_casts': {
        'pattern': r'\(\s*(?:const\s+)?\w+\s*\*\s*\)',
        'description': 'C-style Casts'
    },
    'out_parameters': {
        'pattern': r'\b\w+\s*\*\s*\*\s*\w+',
        'description': 'Out Parameters'
    },
    'null_checks': {
        'pattern': r'(?:if|while)\s*\(\s*(?:!\s*)?\w+\s*(?:==|!=)\s*(?:nullptr|NULL|0)\s*\)',
        'description': 'Null Checks'
    },
    'result_type_usage': {
        'pattern': r'Result<\w+>',
        'description': 'Result Type Usage'
    },
    'smart_pointer_usage': {
        'pattern': r'\b(?:nsCOMPtr|RefPtr|std::unique_ptr|std::shared_ptr)\b',
        'description': 'Smart Pointer Usage'
    },
    'optional_type_usage': {
        'pattern': r'std::optional',
        'description': 'Optional Type Usage'
    }
}

# Code quality metrics
def calculate_cyclomatic_complexity(file_path):
    """Calculate approximate cyclomatic complexity for a file."""
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as file:
        content = file.read()
        
    # Count decision points (simplified approach)
    if_count = len(re.findall(r'\bif\s*\(', content))
    for_count = len(re.findall(r'\bfor\s*\(', content))
    while_count = len(re.findall(r'\bwhile\s*\(', content))
    case_count = len(re.findall(r'\bcase\s+', content))
    catch_count = len(re.findall(r'\bcatch\s*\(', content))
    ternary_count = len(re.findall(r'\?\s*\w+\s*\:', content))
    
    # Basic complexity = 1 + decision points
    complexity = 1 + if_count + for_count + while_count + case_count + catch_count + ternary_count
    
    return complexity

def calculate_function_lengths(file_path):
    """Calculate function lengths in a file."""
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as file:
        content = file.read()
    
    # Find function definitions and count their lines
    function_pattern = r'(\w+(?:\:\:\w+)?)\s*\([^)]*\)\s*(?:const)?\s*\{(?:[^{}]|\{(?:[^{}]|\{[^{}]*\})*\})*\}'
    functions = re.findall(function_pattern, content, re.DOTALL)
    
    lengths = []
    for func_match in re.finditer(function_pattern, content, re.DOTALL):
        func_body = func_match.group(0)
        lines = func_body.count('\n') + 1
        lengths.append(lines)
    
    if not lengths:
        return 0, 0, 0
    
    return sum(lengths) / len(lengths), min(lengths), max(lengths)

def calculate_comment_ratio(file_path):
    """Calculate comment to code ratio."""
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as file:
        content = file.read()
    
    # Count comment lines
    single_line_comments = len(re.findall(r'\/\/.*$', content, re.MULTILINE))
    
    # Count multi-line comments
    multi_line_comments = 0
    for match in re.finditer(r'\/\*.*?\*\/', content, re.DOTALL):
        multi_line_comments += match.group(0).count('\n') + 1
    
    # Count code lines
    total_lines = content.count('\n') + 1
    code_lines = total_lines - single_line_comments - multi_line_comments
    
    if code_lines == 0:
        return 0
    
    return (single_line_comments + multi_line_comments) / code_lines

def count_pattern_occurrences(file_path):
    """Count occurrences of each pattern in a file."""
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as file:
        content = file.read()
    
    results = {}
    for pattern_name, pattern_info in PATTERNS.items():
        matches = re.findall(pattern_info['pattern'], content)
        results[pattern_name] = len(matches)
    
    return results

def analyze_file(file_path):
    """Analyze a single file for all metrics."""
    if not os.path.isfile(file_path):
        return None
    
    try:
        metrics = {
            'file_path': file_path,
            'file_size': os.path.getsize(file_path),
            'patterns': count_pattern_occurrences(file_path),
            'cyclomatic_complexity': calculate_cyclomatic_complexity(file_path),
        }
        
        avg_len, min_len, max_len = calculate_function_lengths(file_path)
        metrics['function_length'] = {
            'average': avg_len,
            'min': min_len,
            'max': max_len
        }
        
        metrics['comment_ratio'] = calculate_comment_ratio(file_path)
        
        return metrics
    except Exception as e:
        print(f"Error analyzing file {file_path}: {e}")
        return None

def find_cpp_files(directory):
    """Find all C/C++ files in a directory."""
    cpp_files = []
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(('.cpp', '.c', '.cc', '.cxx', '.h', '.hpp')):
                cpp_files.append(os.path.join(root, file))
    return cpp_files

def analyze_directory(directory, output_file=None):
    """Analyze all C/C++ files in a directory."""
    cpp_files = find_cpp_files(directory)
    print(f"Found {len(cpp_files)} C/C++ files to analyze.")
    
    results = []
    pattern_totals = defaultdict(int)
    
    for i, file_path in enumerate(cpp_files):
        if i % 10 == 0:
            print(f"Analyzing file {i+1}/{len(cpp_files)}: {file_path}")
        
        metrics = analyze_file(file_path)
        if metrics:
            results.append(metrics)
            for pattern_name, count in metrics['patterns'].items():
                pattern_totals[pattern_name] += count
    
    # Calculate summary statistics
    summary = {
        'total_files': len(results),
        'pattern_totals': dict(pattern_totals),
        'timestamp': datetime.now().isoformat(),
    }
    
    # Save results to file if specified
    if output_file:
        try:
            os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)
            with open(output_file, 'w') as f:
                json.dump({'summary': summary, 'files': results}, f, indent=2)
            print(f"Results saved to {output_file}")
        except Exception as e:
            print(f"Error saving results to {output_file}: {e}")
    
    return summary, results

def main():
    parser = argparse.ArgumentParser(description='Measure modernization KPIs for C/C++ code.')
    parser.add_argument('directory', help='Directory to analyze')
    parser.add_argument('-o', '--output', help='Output JSON file')
    parser.add_argument('-f', '--file', help='Analyze a single file')
    args = parser.parse_args()
    
    if args.file:
        metrics = analyze_file(args.file)
        print(json.dumps(metrics, indent=2))
        
        # Save results to file if specified
        if args.output:
            try:
                os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)
                with open(args.output, 'w') as f:
                    json.dump(metrics, f, indent=2)
                print(f"Results saved to {args.output}")
            except Exception as e:
                print(f"Error saving results to {args.output}: {e}")
    else:
        summary, _ = analyze_directory(args.directory, args.output)
        print("\nSummary:")
        print(f"Total files analyzed: {summary['total_files']}")
        print("\nPattern occurrences:")
        for pattern_name, count in summary['pattern_totals'].items():
            print(f"  {PATTERNS[pattern_name]['description']}: {count}")

if __name__ == "__main__":
    main() 
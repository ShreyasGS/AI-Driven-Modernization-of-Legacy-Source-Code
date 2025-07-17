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
        'pattern': r'(?:mozilla::)?Result\s*<\s*\w+(?:::\w+)*\s*>',
        'description': 'Result Type Usage'
    },
    'smart_pointer_usage': {
        'pattern': r'\b(?:nsCOMPtr|RefPtr|std::unique_ptr|std::shared_ptr)\b',
        'description': 'Smart Pointer Usage'
    },
    'optional_type_usage': {
        'pattern': r'std::optional\s*<\s*\w+(?:::\w+)*\s*>',
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

def measure_documentation_metrics():
    """Measure documentation metrics for the modernization project."""
    doc_metrics = {
        'doc_files': 0,
        'doc_lines': 0,
        'modernization_doc_files': 0,
        'modernization_doc_lines': 0,
        'template_doc_files': 0,
        'template_doc_lines': 0,
        'code_doc_files': 0,        # New metric for files with code documentation
        'code_doc_lines': 0         # New metric for lines of code documentation
    }
    
    # Count documentation files
    doc_files = []
    for root, _, files in os.walk('.'):
        for file in files:
            if file.endswith('.md'):
                file_path = os.path.join(root, file)
                doc_files.append(file_path)
                
                # Count lines in documentation files
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        lines = content.count('\n') + 1
                        doc_metrics['doc_files'] += 1
                        doc_metrics['doc_lines'] += lines
                        
                        # Categorize documentation files
                        if 'modernization_' in file or 'modernized_' in file:
                            doc_metrics['modernization_doc_files'] += 1
                            doc_metrics['modernization_doc_lines'] += lines
                        elif 'modernization_templates' in file_path:
                            doc_metrics['template_doc_files'] += 1
                            doc_metrics['template_doc_lines'] += lines
                except Exception as e:
                    print(f"Error reading documentation file {file_path}: {e}")
    
    # Count code documentation in implementation files
    code_doc_metrics = measure_code_documentation()
    doc_metrics['code_doc_files'] = code_doc_metrics['files']
    doc_metrics['code_doc_lines'] = code_doc_metrics['lines']
    
    return doc_metrics

def measure_code_documentation():
    """Measure documentation within code files (comments and docstrings)."""
    metrics = {
        'files': 0,
        'lines': 0
    }
    
    # Keep track of processed files
    processed_files = set()
    
    # Look for modernized implementation files
    for root, _, files in os.walk('.'):
        for file in files:
            if (file.startswith('modernized_') or 'modernized' in file) and file.endswith(('.cpp', '.h')):
                file_path = os.path.join(root, file)
                
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        has_documentation = False
                        
                        # Count documentation comments (/** ... */ style and /* ... */ style)
                        doc_comments = re.findall(r'/\*\*(.*?)\*/', content, re.DOTALL)
                        doc_comments.extend(re.findall(r'/\*(.*?)\*/', content, re.DOTALL))
                        
                        # If we found documentation comments, count this file
                        if doc_comments:
                            has_documentation = True
                            # Count lines in documentation comments
                            for comment in doc_comments:
                                lines = comment.count('\n') + 1
                                metrics['lines'] += lines
                                
                        # Also check for line comments that document functions
                        line_comments = re.findall(r'//\s*.*\n', content)
                        if line_comments:
                            has_documentation = True
                            # Count lines in line comments
                            for comment in line_comments:
                                metrics['lines'] += 1
                        
                        # Only count the file once if it has documentation
                        if has_documentation and file_path not in processed_files:
                            metrics['files'] += 1
                            processed_files.add(file_path)
                            
                except Exception as e:
                    print(f"Error analyzing code documentation in {file_path}: {e}")
    
    return metrics

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

def find_modernized_files(directory):
    """Find all modernized implementation files."""
    modernized_files = []
    for root, _, files in os.walk(directory):
        for file in files:
            if (file.startswith('modernized_') or 'modernized' in file) and file.endswith(('.cpp', '.h')):
                modernized_files.append(os.path.join(root, file))
    return modernized_files

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
    
    # Specifically analyze modernized files for Result and Optional types
    modernized_files = find_modernized_files(directory)
    print(f"Found {len(modernized_files)} modernized files to analyze.")
    
    modernized_pattern_totals = {
        'result_type_usage': 0,
        'optional_type_usage': 0
    }
    
    for file_path in modernized_files:
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as file:
                content = file.read()
                
                # Count Result type usage
                result_matches = re.findall(PATTERNS['result_type_usage']['pattern'], content)
                modernized_pattern_totals['result_type_usage'] += len(result_matches)
                
                # Count Optional type usage
                optional_matches = re.findall(PATTERNS['optional_type_usage']['pattern'], content)
                modernized_pattern_totals['optional_type_usage'] += len(optional_matches)
        except Exception as e:
            print(f"Error analyzing modernized file {file_path}: {e}")
    
    # Add modernized pattern counts to the totals
    pattern_totals['result_type_usage'] = modernized_pattern_totals['result_type_usage']
    pattern_totals['optional_type_usage'] = modernized_pattern_totals['optional_type_usage']
    
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
    parser.add_argument('-d', '--doc-metrics', action='store_true', help='Include documentation metrics')
    args = parser.parse_args()
    
    # Measure documentation metrics if requested
    doc_metrics = None
    if args.doc_metrics:
        print("Measuring documentation metrics...")
        doc_metrics = measure_documentation_metrics()
        print(f"Documentation files: {doc_metrics['doc_files']}")
        print(f"Documentation lines: {doc_metrics['doc_lines']}")
        print(f"Modernization documentation files: {doc_metrics['modernization_doc_files']}")
        print(f"Modernization documentation lines: {doc_metrics['modernization_doc_lines']}")
        print(f"Template documentation files: {doc_metrics['template_doc_files']}")
        print(f"Template documentation lines: {doc_metrics['template_doc_lines']}")
        print(f"Code documentation files: {doc_metrics['code_doc_files']}")
        print(f"Code documentation lines: {doc_metrics['code_doc_lines']}")
    
    if args.file:
        metrics = analyze_file(args.file)
        if metrics is None:
            print(f"Error: Could not analyze file {args.file}")
            return
            
        if doc_metrics:
            metrics['documentation_metrics'] = doc_metrics
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
        summary, results = analyze_directory(args.directory, args.output)
        if doc_metrics:
            summary['documentation_metrics'] = doc_metrics
            
            # If output file was specified, update it with documentation metrics
            if args.output:
                try:
                    with open(args.output, 'r') as f:
                        data = json.load(f)
                    data['summary']['documentation_metrics'] = doc_metrics
                    with open(args.output, 'w') as f:
                        json.dump(data, f, indent=2)
                except Exception as e:
                    print(f"Error updating output file with documentation metrics: {e}")
                    
        print("\nSummary:")
        print(f"Total files analyzed: {summary['total_files']}")
        print("\nPattern occurrences:")
        for pattern_name, count in summary['pattern_totals'].items():
            print(f"  {PATTERNS[pattern_name]['description']}: {count}")
        
        if doc_metrics:
            print("\nDocumentation metrics:")
            print(f"  Documentation files: {doc_metrics['doc_files']}")
            print(f"  Documentation lines: {doc_metrics['doc_lines']}")
            print(f"  Modernization documentation files: {doc_metrics['modernization_doc_files']}")
            print(f"  Modernization documentation lines: {doc_metrics['modernization_doc_lines']}")
            print(f"  Template documentation files: {doc_metrics['template_doc_files']}")
            print(f"  Template documentation lines: {doc_metrics['template_doc_lines']}")
            print(f"  Code documentation files: {doc_metrics['code_doc_files']}")
            print(f"  Code documentation lines: {doc_metrics['code_doc_lines']}")

if __name__ == "__main__":
    main() 
#!/usr/bin/env python3
"""
Modernization KPI Report Generator

This script generates a simple text-based report of modernization KPIs.
"""

import os
import json
import argparse
from datetime import datetime

def load_data(json_file):
    """Load data from JSON file."""
    with open(json_file, 'r') as f:
        return json.load(f)

def generate_text_report(kpi_data, progress_data=None, output_file=None):
    """Generate a text-based report from KPI and progress data."""
    
    # Extract metrics
    patterns = kpi_data.get('patterns', {})
    function_length = kpi_data.get('function_length', {})
    cyclomatic_complexity = kpi_data.get('cyclomatic_complexity', 0)
    comment_ratio = kpi_data.get('comment_ratio', 0)
    doc_metrics = kpi_data.get('documentation_metrics', {})
    
    # Start building the report
    report = []
    report.append("# Mozilla 1.0 Modernization KPI Report")
    report.append(f"Generated on: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    report.append("")
    
    report.append("## Code Quality Metrics")
    report.append(f"- Cyclomatic Complexity: {cyclomatic_complexity}")
    report.append(f"- Average Function Length: {function_length.get('average', 0):.1f} lines")
    report.append(f"- Min Function Length: {function_length.get('min', 0)} lines")
    report.append(f"- Max Function Length: {function_length.get('max', 0)} lines")
    report.append(f"- Comment-to-Code Ratio: {comment_ratio:.2f}")
    report.append("")
    
    report.append("## Code Pattern Distribution")
    for pattern_name, count in patterns.items():
        report.append(f"- {pattern_name.replace('_', ' ').title()}: {count}")
    report.append("")
    
    # Add documentation metrics if available
    if doc_metrics:
        report.append("## Documentation Metrics")
        report.append(f"- Total Documentation Files: {doc_metrics.get('doc_files', 0)}")
        report.append(f"- Total Documentation Lines: {doc_metrics.get('doc_lines', 0)}")
        report.append(f"- Modernization Documentation Files: {doc_metrics.get('modernization_doc_files', 0)}")
        report.append(f"- Modernization Documentation Lines: {doc_metrics.get('modernization_doc_lines', 0)}")
        report.append(f"- Template Documentation Files: {doc_metrics.get('template_doc_files', 0)}")
        report.append(f"- Template Documentation Lines: {doc_metrics.get('template_doc_lines', 0)}")
        report.append("")
    
    # Add progress information if available
    if progress_data:
        total_methods = progress_data.get('total_methods', 0)
        modernized_methods = progress_data.get('modernized_methods', 0)
        progress_percentage = progress_data.get('method_progress_percentage', 0)
        
        report.append("## Modernization Progress")
        report.append(f"- Total Methods: {total_methods}")
        report.append(f"- Modernized Methods: {modernized_methods}")
        report.append(f"- Progress Percentage: {progress_percentage:.1f}%")
        report.append("")
        
        report.append("## Modernized Methods")
        modernized_methods_list = progress_data.get('modernized_methods_list', [])
        for method in modernized_methods_list:
            report.append(f"- {method}")
        report.append("")
        
        # Add pattern reduction information
        original_patterns = progress_data.get('original_pattern_total', 0)
        modernized_patterns = progress_data.get('modernized_patterns_total', 0)
        
        report.append("## Pattern Reduction")
        report.append(f"- Original Pattern Occurrences: {original_patterns}")
        report.append(f"- Modernized Pattern Implementations: {modernized_patterns}")
        report.append("")
    
    report.append("## Next Steps")
    report.append("1. Continue applying modernization templates to remaining methods")
    report.append("2. Expand modernization to other high-priority files")
    report.append("3. Conduct regular KPI measurements to track progress")
    report.append("")
    
    # Join the report lines
    report_text = "\n".join(report)
    
    # Write to file if specified
    if output_file:
        with open(output_file, 'w') as f:
            f.write(report_text)
        print(f"Report saved to {output_file}")
    
    return report_text

def main():
    parser = argparse.ArgumentParser(description='Generate modernization KPI report')
    parser.add_argument('kpi_file', help='KPI data JSON file')
    parser.add_argument('-p', '--progress', help='Progress data JSON file')
    parser.add_argument('-o', '--output', help='Output report file')
    args = parser.parse_args()
    
    kpi_data = load_data(args.kpi_file)
    
    progress_data = None
    if args.progress and os.path.exists(args.progress):
        progress_data = load_data(args.progress)
    
    report = generate_text_report(kpi_data, progress_data, args.output)
    
    # Print report to console if no output file specified
    if not args.output:
        print(report)

if __name__ == "__main__":
    main() 
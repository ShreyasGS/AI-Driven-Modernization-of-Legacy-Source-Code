#!/usr/bin/env python3
"""
Modernization Progress Dashboard Generator

This script generates an HTML dashboard to visualize modernization progress.
"""

import os
import json
import argparse
from datetime import datetime
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('Agg')  # Use non-interactive backend

def load_kpi_data(json_file):
    """Load KPI data from JSON file."""
    with open(json_file, 'r') as f:
        return json.load(f)

def generate_html_dashboard(kpi_data, progress_data, output_file):
    """Generate HTML dashboard from KPI data."""
    
    # Extract metrics
    patterns = kpi_data.get('patterns', {})
    function_length = kpi_data.get('function_length', {})
    cyclomatic_complexity = kpi_data.get('cyclomatic_complexity', 0)
    comment_ratio = kpi_data.get('comment_ratio', 0)
    
    # Create charts directory if it doesn't exist
    charts_dir = os.path.join(os.path.dirname(output_file), 'charts')
    os.makedirs(charts_dir, exist_ok=True)
    
    # Generate pattern distribution chart
    pattern_chart_path = os.path.join(charts_dir, 'pattern_distribution.png')
    generate_pattern_chart(patterns, pattern_chart_path)
    
    # Generate method progress chart if progress data is available
    method_progress_chart_path = None
    if progress_data:
        method_progress_chart_path = os.path.join(charts_dir, 'method_progress.png')
        generate_method_progress_chart(progress_data, method_progress_chart_path)
    
    # Generate HTML content
    html_content = f"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Mozilla 1.0 Modernization Dashboard</title>
        <style>
            body {{
                font-family: Arial, sans-serif;
                margin: 0;
                padding: 20px;
                color: #333;
                background-color: #f5f5f5;
            }}
            .container {{
                max-width: 1200px;
                margin: 0 auto;
                background-color: white;
                padding: 20px;
                border-radius: 5px;
                box-shadow: 0 2px 5px rgba(0,0,0,0.1);
            }}
            h1, h2, h3 {{
                color: #2a56c6;
            }}
            .header {{
                display: flex;
                justify-content: space-between;
                align-items: center;
                margin-bottom: 20px;
                padding-bottom: 10px;
                border-bottom: 1px solid #ddd;
            }}
            .timestamp {{
                color: #666;
                font-size: 0.9em;
            }}
            .metrics-container {{
                display: flex;
                flex-wrap: wrap;
                gap: 20px;
                margin-bottom: 30px;
            }}
            .metric-card {{
                flex: 1;
                min-width: 200px;
                background-color: #f9f9f9;
                border-radius: 5px;
                padding: 15px;
                box-shadow: 0 1px 3px rgba(0,0,0,0.1);
            }}
            .metric-value {{
                font-size: 24px;
                font-weight: bold;
                color: #2a56c6;
                margin: 10px 0;
            }}
            .chart-container {{
                margin-top: 30px;
            }}
            table {{
                width: 100%;
                border-collapse: collapse;
                margin-top: 20px;
            }}
            th, td {{
                padding: 12px 15px;
                text-align: left;
                border-bottom: 1px solid #ddd;
            }}
            th {{
                background-color: #f2f2f2;
            }}
            tr:hover {{
                background-color: #f5f5f5;
            }}
            .progress-bar {{
                height: 20px;
                background-color: #e0e0e0;
                border-radius: 10px;
                overflow: hidden;
                margin-top: 5px;
            }}
            .progress-bar-fill {{
                height: 100%;
                background-color: #4CAF50;
                border-radius: 10px;
            }}
        </style>
    </head>
    <body>
        <div class="container">
            <div class="header">
                <h1>Mozilla 1.0 Modernization Dashboard</h1>
                <span class="timestamp">Generated on {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</span>
            </div>
            
            <h2>nsSelection.cpp Modernization Progress</h2>
            
            <div class="metrics-container">
                <div class="metric-card">
                    <h3>Cyclomatic Complexity</h3>
                    <div class="metric-value">{cyclomatic_complexity}</div>
                    <div>Target: 20% reduction</div>
                </div>
                <div class="metric-card">
                    <h3>Average Function Length</h3>
                    <div class="metric-value">{function_length.get('average', 0):.1f} lines</div>
                    <div>Target: 25% reduction</div>
                </div>
                <div class="metric-card">
                    <h3>Comment Ratio</h3>
                    <div class="metric-value">{comment_ratio:.2f}</div>
                    <div>Target: 15% increase</div>
                </div>
                <div class="metric-card">
                    <h3>File Size</h3>
                    <div class="metric-value">{kpi_data.get('file_size', 0) / 1024:.1f} KB</div>
                </div>
            </div>
            
            <div class="chart-container">
                <h3>Code Pattern Distribution</h3>
                <img src="charts/pattern_distribution.png" alt="Pattern Distribution" style="max-width: 100%;">
            </div>
    """
    
    if method_progress_chart_path:
        html_content += f"""
            <div class="chart-container">
                <h3>Method Modernization Progress</h3>
                <img src="charts/method_progress.png" alt="Method Progress" style="max-width: 100%;">
            </div>
        """
    
    if progress_data:
        total_methods = progress_data.get('total_methods', 0)
        modernized_methods = progress_data.get('modernized_methods', 0)
        progress_percentage = progress_data.get('method_progress_percentage', 0)
        
        html_content += f"""
            <h3>Modernization Coverage</h3>
            <div>
                <strong>Methods Modernized:</strong> {modernized_methods} / {total_methods} ({progress_percentage:.1f}%)
                <div class="progress-bar">
                    <div class="progress-bar-fill" style="width: {progress_percentage}%;"></div>
                </div>
            </div>
            
            <h3>Modernized Methods</h3>
            <table>
                <tr>
                    <th>Method Name</th>
                    <th>Status</th>
                </tr>
        """
        
        modernized_methods_list = progress_data.get('modernized_methods_list', [])
        for method in modernized_methods_list:
            html_content += f"""
                <tr>
                    <td>{method}</td>
                    <td>✅ Modernized</td>
                </tr>
            """
        
        html_content += """
            </table>
        """
    
    html_content += """
        </div>
    </body>
    </html>
    """
    
    with open(output_file, 'w') as f:
        f.write(html_content)
    
    print(f"Dashboard generated at {output_file}")

def generate_pattern_chart(patterns, output_file):
    """Generate a bar chart for pattern distribution."""
    labels = []
    values = []
    
    for pattern, count in patterns.items():
        if pattern in ['manual_reference_counting', 'error_code_returns', 'c_style_casts', 'out_parameters', 'null_checks']:
            labels.append(pattern.replace('_', ' ').title())
            values.append(count)
    
    plt.figure(figsize=(10, 6))
    bars = plt.bar(labels, values, color='#2a56c6')
    
    plt.title('Original Code Pattern Distribution')
    plt.xlabel('Pattern Type')
    plt.ylabel('Count')
    plt.xticks(rotation=45, ha='right')
    
    # Add count labels on top of bars
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height + 0.1,
                 f'{int(height)}', ha='center', va='bottom')
    
    plt.tight_layout()
    plt.savefig(output_file)
    plt.close()

def generate_method_progress_chart(progress_data, output_file):
    """Generate a pie chart for method modernization progress."""
    total_methods = progress_data.get('total_methods', 0)
    modernized_methods = progress_data.get('modernized_methods', 0)
    remaining_methods = total_methods - modernized_methods
    
    labels = ['Modernized', 'Remaining']
    sizes = [modernized_methods, remaining_methods]
    colors = ['#4CAF50', '#f0f0f0']
    explode = (0.1, 0)  # explode the 1st slice (Modernized)
    
    plt.figure(figsize=(8, 8))
    plt.pie(sizes, explode=explode, labels=labels, colors=colors,
            autopct='%1.1f%%', shadow=True, startangle=90)
    plt.axis('equal')  # Equal aspect ratio ensures that pie is drawn as a circle
    plt.title('Method Modernization Progress')
    
    plt.savefig(output_file)
    plt.close()

def main():
    parser = argparse.ArgumentParser(description='Generate modernization progress dashboard')
    parser.add_argument('kpi_file', help='KPI data JSON file')
    parser.add_argument('-p', '--progress', help='Progress data JSON file')
    parser.add_argument('-o', '--output', default='modernization_dashboard.html', help='Output HTML file')
    args = parser.parse_args()
    
    kpi_data = load_kpi_data(args.kpi_file)
    
    progress_data = None
    if args.progress and os.path.exists(args.progress):
        progress_data = load_kpi_data(args.progress)
    
    generate_html_dashboard(kpi_data, progress_data, args.output)

if __name__ == "__main__":
    main() 
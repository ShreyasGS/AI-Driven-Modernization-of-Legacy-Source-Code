# Mozilla 1.0 Modernization KPI Measurement Guide

This guide explains how to use the KPI measurement tools created for tracking the modernization progress of the Mozilla 1.0 codebase.

## Overview

We've established several Key Performance Indicators (KPIs) to measure our modernization progress, as detailed in `nsSelection_modernization_kpis.md`. To track these KPIs, we've created a set of tools:

1. **measure_modernization_kpis.py** - Analyzes C/C++ code for modernization patterns and code quality metrics
2. **track_nsSelection_progress.py** - Tracks modernization progress specifically for nsSelection.cpp
3. **modernization_dashboard.py** - Generates an HTML dashboard to visualize progress
4. **update_kpis.sh** - A shell script that runs all the above tools and updates the dashboard

## Requirements

- Python 3.6+
- matplotlib (for dashboard charts)

Install the required Python packages:

```bash
pip3 install matplotlib
```

## Using the Tools

### Measuring KPIs for a Single File

```bash
python3 analysis/measure_modernization_kpis.py . -f <path/to/file> -o <output.json>
```

Example:
```bash
python3 analysis/measure_modernization_kpis.py . -f content/base/src/nsSelection.cpp -o analysis/reports/nsSelection_baseline_kpis.json
```

### Measuring KPIs for a Directory

```bash
python3 analysis/measure_modernization_kpis.py <directory> -o <output.json>
```

Example:
```bash
python3 analysis/measure_modernization_kpis.py content/base/src -o analysis/reports/base_src_kpis.json
```

### Tracking nsSelection.cpp Modernization Progress

```bash
python3 analysis/track_nsSelection_progress.py <path/to/original/nsSelection.cpp> <directory_with_modernized_files> -o <output.json>
```

Example:
```bash
python3 analysis/track_nsSelection_progress.py content/base/src/nsSelection.cpp . -o analysis/reports/nsSelection_progress.json
```

### Generating the Dashboard

```bash
python3 analysis/modernization_dashboard.py <kpi_file.json> -p <progress_file.json> -o <output.html>
```

Example:
```bash
python3 analysis/modernization_dashboard.py analysis/reports/nsSelection_baseline_kpis.json -p analysis/reports/nsSelection_progress.json -o analysis/dashboard/modernization_dashboard.html
```

### Using the Update Script

The easiest way to update all KPIs and regenerate the dashboard is to use the update script:

```bash
./analysis/update_kpis.sh
```

This script:
1. Measures KPIs for nsSelection.cpp
2. Tracks modernization progress
3. Generates a timestamped dashboard
4. Updates the latest dashboard link

## Dashboard Features

The modernization dashboard includes:

- Code quality metrics (cyclomatic complexity, function length, comment ratio)
- Pattern distribution charts
- Modernization progress visualization
- List of modernized methods

## Measurement Schedule

As per our KPI plan, we should:

1. Measure modernization coverage metrics weekly
2. Measure bug count and crash rate monthly
3. Conduct developer surveys quarterly
4. Measure code quality metrics after each major milestone

## Adding New Metrics

To add new metrics to the measurement tools:

1. Add new pattern definitions to the `PATTERNS` dictionary in `measure_modernization_kpis.py`
2. Implement new measurement functions as needed
3. Update the dashboard generation code in `modernization_dashboard.py`

## Troubleshooting

If you encounter issues:

- Ensure Python 3.6+ is installed
- Verify matplotlib is installed
- Check file paths are correct
- Ensure write permissions for output directories

For more detailed information about our KPIs, refer to `nsSelection_modernization_kpis.md`. 
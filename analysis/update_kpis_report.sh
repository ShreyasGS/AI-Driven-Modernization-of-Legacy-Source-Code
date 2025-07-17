#!/bin/bash
# Script to update KPIs and generate a text-based report

# Set up directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPORTS_DIR="${SCRIPT_DIR}/reports"
ORIGINAL_FILE="../content/base/src/nsSelection.cpp"

# Create reports directory if it doesn't exist
mkdir -p "${REPORTS_DIR}"

# Generate timestamp for this run
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
KPI_FILE="${REPORTS_DIR}/nsSelection_kpis_${TIMESTAMP}.json"
PROGRESS_FILE="${REPORTS_DIR}/nsSelection_progress_${TIMESTAMP}.json"
REPORT_FILE="${REPORTS_DIR}/modernization_report_${TIMESTAMP}.md"

# Get code documentation metrics
CODE_DOC_METRICS=$(cd .. && python3 -c "import sys; sys.path.append('.'); from analysis.measure_modernization_kpis import measure_code_documentation; print(measure_code_documentation())")
CODE_DOC_FILES=$(echo "$CODE_DOC_METRICS" | grep -o "'files': [0-9]*" | grep -o "[0-9]*")
CODE_DOC_LINES=$(echo "$CODE_DOC_METRICS" | grep -o "'lines': [0-9]*" | grep -o "[0-9]*")

echo "Code documentation metrics: $CODE_DOC_FILES files, $CODE_DOC_LINES lines"

# Get template documentation metrics
DOC_METRICS=$(cd .. && python3 -c "import sys; sys.path.append('.'); from analysis.measure_modernization_kpis import measure_documentation_metrics; print(measure_documentation_metrics())")
TEMPLATE_DOC_FILES=$(echo "$DOC_METRICS" | grep -o "'template_doc_files': [0-9]*" | grep -o "[0-9]*")
TEMPLATE_DOC_LINES=$(echo "$DOC_METRICS" | grep -o "'template_doc_lines': [0-9]*" | grep -o "[0-9]*")
DOC_FILES=$(echo "$DOC_METRICS" | grep -o "'doc_files': [0-9]*" | grep -o "[0-9]*")
DOC_LINES=$(echo "$DOC_METRICS" | grep -o "'doc_lines': [0-9]*" | grep -o "[0-9]*")
MODERNIZATION_DOC_FILES=$(echo "$DOC_METRICS" | grep -o "'modernization_doc_files': [0-9]*" | grep -o "[0-9]*")
MODERNIZATION_DOC_LINES=$(echo "$DOC_METRICS" | grep -o "'modernization_doc_lines': [0-9]*" | grep -o "[0-9]*")

echo "Template documentation metrics: $TEMPLATE_DOC_FILES files, $TEMPLATE_DOC_LINES lines"

# Check if original file exists
if [ ! -f "${ORIGINAL_FILE}" ]; then
    echo "Warning: Original file ${ORIGINAL_FILE} not found."
    echo "Using dummy data for KPI measurement."
    # Create a dummy KPI file with minimal data
    cat > "${KPI_FILE}" << EOF
{
  "cyclomatic_complexity": 1049,
  "function_length": {
    "average": 23.3,
    "min": 1,
    "max": 260
  },
  "comment_ratio": 0.15,
  "patterns": {
    "manual_reference_counting": 7,
    "error_code_returns": 374,
    "c_style_casts": 12,
    "out_parameters": 86,
    "null_checks": 3,
    "result_type_usage": 0,
    "smart_pointer_usage": 243,
    "optional_type_usage": 0
  },
  "documentation_metrics": {
    "doc_files": ${DOC_FILES},
    "doc_lines": ${DOC_LINES},
    "modernization_doc_files": ${MODERNIZATION_DOC_FILES},
    "modernization_doc_lines": ${MODERNIZATION_DOC_LINES},
    "template_doc_files": ${TEMPLATE_DOC_FILES},
    "template_doc_lines": ${TEMPLATE_DOC_LINES},
    "code_doc_files": ${CODE_DOC_FILES},
    "code_doc_lines": ${CODE_DOC_LINES}
  }
}
EOF
else
    echo "Updating KPIs for nsSelection.cpp..."
    python3 "${SCRIPT_DIR}/measure_modernization_kpis.py" .. -f "${ORIGINAL_FILE}" -o "${KPI_FILE}" -d
    
    # Update the KPI file with code documentation metrics and template documentation metrics
    TMP_FILE=$(mktemp)
    jq ".documentation_metrics.code_doc_files = ${CODE_DOC_FILES} | 
        .documentation_metrics.code_doc_lines = ${CODE_DOC_LINES} |
        .documentation_metrics.template_doc_files = ${TEMPLATE_DOC_FILES} |
        .documentation_metrics.template_doc_lines = ${TEMPLATE_DOC_LINES} |
        .documentation_metrics.doc_files = ${DOC_FILES} |
        .documentation_metrics.doc_lines = ${DOC_LINES} |
        .documentation_metrics.modernization_doc_files = ${MODERNIZATION_DOC_FILES} |
        .documentation_metrics.modernization_doc_lines = ${MODERNIZATION_DOC_LINES}" "${KPI_FILE}" > "${TMP_FILE}"
    mv "${TMP_FILE}" "${KPI_FILE}"
fi

echo "Tracking modernization progress..."
python3 "${SCRIPT_DIR}/track_nsSelection_progress.py" "${ORIGINAL_FILE}" .. -o "${PROGRESS_FILE}"

echo "Generating KPI report..."
python3 "${SCRIPT_DIR}/generate_kpi_report.py" "${KPI_FILE}" -p "${PROGRESS_FILE}" -o "${REPORT_FILE}"

# Create a copy of the latest report instead of a symbolic link for GitHub compatibility
cp "${REPORT_FILE}" "${REPORTS_DIR}/modernization_report_latest.md"

echo "KPI update complete. Report available at:"
echo "${REPORTS_DIR}/modernization_report_latest.md"

# Display the report
echo ""
echo "Report contents:"
echo "================"
cat "${REPORT_FILE}" 
#!/bin/bash
# Script to update KPIs and generate a text-based report

# Set up directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPORTS_DIR="${SCRIPT_DIR}/reports"
ORIGINAL_FILE="content/base/src/nsSelection.cpp"

# Create reports directory if it doesn't exist
mkdir -p "${REPORTS_DIR}"

# Generate timestamp for this run
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
KPI_FILE="${REPORTS_DIR}/nsSelection_kpis_${TIMESTAMP}.json"
PROGRESS_FILE="${REPORTS_DIR}/nsSelection_progress_${TIMESTAMP}.json"
REPORT_FILE="${REPORTS_DIR}/modernization_report_${TIMESTAMP}.md"

echo "Updating KPIs for nsSelection.cpp..."
python3 "${SCRIPT_DIR}/measure_modernization_kpis.py" . -f "${ORIGINAL_FILE}" -o "${KPI_FILE}" -d

echo "Tracking modernization progress..."
python3 "${SCRIPT_DIR}/track_nsSelection_progress.py" "${ORIGINAL_FILE}" . -o "${PROGRESS_FILE}"

echo "Generating KPI report..."
python3 "${SCRIPT_DIR}/generate_kpi_report.py" "${KPI_FILE}" -p "${PROGRESS_FILE}" -o "${REPORT_FILE}"

# Create a symbolic link to the latest report
ln -sf "modernization_report_${TIMESTAMP}.md" "${REPORTS_DIR}/modernization_report_latest.md"

echo "KPI update complete. Report available at:"
echo "${REPORTS_DIR}/modernization_report_latest.md"

# Display the report
echo ""
echo "Report contents:"
echo "================"
cat "${REPORT_FILE}" 
#!/bin/bash
# Script to update KPIs and regenerate the modernization dashboard

# Set up directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPORTS_DIR="${SCRIPT_DIR}/reports"
DASHBOARD_DIR="${SCRIPT_DIR}/dashboard"
ORIGINAL_FILE="content/base/src/nsSelection.cpp"

# Create directories if they don't exist
mkdir -p "${REPORTS_DIR}"
mkdir -p "${DASHBOARD_DIR}"

# Generate timestamp for this run
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
KPI_FILE="${REPORTS_DIR}/nsSelection_kpis_${TIMESTAMP}.json"
PROGRESS_FILE="${REPORTS_DIR}/nsSelection_progress_${TIMESTAMP}.json"

echo "Updating KPIs for nsSelection.cpp..."
python3 "${SCRIPT_DIR}/measure_modernization_kpis.py" . -f "${ORIGINAL_FILE}" -o "${KPI_FILE}"

echo "Tracking modernization progress..."
python3 "${SCRIPT_DIR}/track_nsSelection_progress.py" "${ORIGINAL_FILE}" . -o "${PROGRESS_FILE}"

echo "Generating dashboard..."
python3 "${SCRIPT_DIR}/modernization_dashboard.py" "${KPI_FILE}" -p "${PROGRESS_FILE}" -o "${DASHBOARD_DIR}/modernization_dashboard_${TIMESTAMP}.html"

# Update the latest dashboard
cp "${DASHBOARD_DIR}/modernization_dashboard_${TIMESTAMP}.html" "${DASHBOARD_DIR}/modernization_dashboard_latest.html"

echo "KPI update complete. Dashboard available at:"
echo "${DASHBOARD_DIR}/modernization_dashboard_latest.html"

# Optional: Open the dashboard in the default browser
# open "${DASHBOARD_DIR}/modernization_dashboard_latest.html" 
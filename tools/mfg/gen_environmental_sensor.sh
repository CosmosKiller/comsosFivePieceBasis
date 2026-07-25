#!/usr/bin/env bash
# Generate factory partition + onboarding codes for iotEnvironmentalSensor.
# PID 0x8003 | ESP32-C5 | Matter environmental sensor (BME680)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

SKU_KEY="environmental_sensor"
SKU_CODE="ENV"
SKU_NAME="iotEnvironmentalSensor"
CN_PREFIX="cosmos-env"
PID="0x8003"
PRODUCT_NAME="iotEnvironmentalSensor"
HW_VER="0"
HW_VER_STR="env-1.0-beta.1"
IDF_TARGET="esp32c5"

# No Chip-Test CD-8003 yet; CD-8002 + NoPID PAI is acceptable for closed beta.
# Replace CD path with your CSA CD when the product is certified.
PAI_KEY="${ATT}/Chip-Test-PAI-FFF2-NoPID-Key.pem"
PAI_CERT="${ATT}/Chip-Test-PAI-FFF2-NoPID-Cert.pem"
CD_FILE="${CD}/Chip-Test-CD-FFF2-8002.der"

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage_count "$(basename "$0")"
    exit 0
fi

parse_args "$@"
run_mfg

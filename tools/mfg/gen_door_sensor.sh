#!/usr/bin/env bash
# Generate factory partition + onboarding codes for iotDoorSensor.
# PID 0x8001 | ESP32-C6 | Matter door/window contact sensor
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

SKU_KEY="door_sensor"
SKU_CODE="DOOR"
SKU_NAME="iotDoorSensor"
CN_PREFIX="cosmos-door"
PID="0x8001"
PRODUCT_NAME="iotDoorSensor"
HW_VER="0"
HW_VER_STR="door-1.0-beta.1"
IDF_TARGET="esp32c6"

PAI_KEY="${ATT}/Chip-Test-PAI-FFF2-8001-Key.pem"
PAI_CERT="${ATT}/Chip-Test-PAI-FFF2-8001-Cert.pem"
CD_FILE="${CD}/Chip-Test-CD-FFF2-8001.der"

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage_count "$(basename "$0")"
    exit 0
fi

parse_args "$@"
run_mfg

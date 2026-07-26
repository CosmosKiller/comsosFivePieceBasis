#!/usr/bin/env bash
# Generate factory partition + onboarding codes for iotDoorIntercom.
# PID 0x8005 | ESP32-S3 | Matter door intercom (doorbell + PIR + MJPEG)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

SKU_KEY="door_intercom"
SKU_CODE="INTR"
SKU_NAME="iotDoorIntercom"
CN_PREFIX="cosmos-intercom"
PID="0x8005"
PRODUCT_NAME="iotDoorIntercom"
HW_VER="0"
HW_VER_STR="intercom-1.0-beta.1"
IDF_TARGET="esp32s3"

# No Chip-Test CD-8005 yet; CD-8002 + NoPID PAI is acceptable for closed beta.
PAI_KEY="${ATT}/Chip-Test-PAI-FFF2-NoPID-Key.pem"
PAI_CERT="${ATT}/Chip-Test-PAI-FFF2-NoPID-Cert.pem"
CD_FILE="${CD}/Chip-Test-CD-FFF2-8002.der"

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage_count "$(basename "$0")"
    exit 0
fi

parse_args "$@"
run_mfg

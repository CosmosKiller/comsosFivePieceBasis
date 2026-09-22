#!/usr/bin/env bash
# Generate factory partition + onboarding codes for iotDoorIntercom.
# PID 0x8005 | Matter door intercom (doorbell + PIR + tamper + siren; Matter 1.5 cam)
# Product: Waveshare ESP32-P4-WIFI6 — dual image (esp32p4 media/I/O + esp32c6 Matter).
# Factory tool target for the P4-side app image until dual-image mfg lands.
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
IDF_TARGET="esp32p4"

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

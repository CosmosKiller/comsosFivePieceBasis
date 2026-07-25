#!/usr/bin/env bash
# Generate factory partition + onboarding codes for iotDualModeBtn.
# PID 0x8002 | ESP32-C6 | Matter switch (Wi-Fi + Thread firmware — see MANUFACTURING.md)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

SKU_KEY="dual_mode_btn"
SKU_CODE="BTN"
SKU_NAME="iotDualModeBtn"
CN_PREFIX="cosmos-btn"
PID="0x8002"
PRODUCT_NAME="iotDualModeBtn"
HW_VER="0"
HW_VER_STR="btn-1.0-beta.1"
IDF_TARGET="esp32c6"

# No Chip-Test PAI-8002 in the SDK; NoPID PAI works with CD-8002 for beta batches.
PAI_KEY="${ATT}/Chip-Test-PAI-FFF2-NoPID-Key.pem"
PAI_CERT="${ATT}/Chip-Test-PAI-FFF2-NoPID-Cert.pem"
CD_FILE="${CD}/Chip-Test-CD-FFF2-8002.der"

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage_count "$(basename "$0")"
    exit 0
fi

parse_args "$@"
run_mfg

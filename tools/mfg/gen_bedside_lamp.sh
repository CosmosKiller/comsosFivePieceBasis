#!/usr/bin/env bash
# Generate factory partition + onboarding codes for iotBedsideLamp.
# PID 0x8004 | ESP32-C6 | Matter extended color light
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

SKU_KEY="bedside_lamp"
SKU_CODE="LAMP"
SKU_NAME="iotBedsideLamp"
CN_PREFIX="cosmos-lamp"
PID="0x8004"
PRODUCT_NAME="iotBedsideLamp"
HW_VER="0"
HW_VER_STR="lamp-1.0-beta.1"
IDF_TARGET="esp32c6"

# No Chip-Test CD-8004 yet; CD-8002 + NoPID PAI is acceptable for closed beta.
PAI_KEY="${ATT}/Chip-Test-PAI-FFF2-NoPID-Key.pem"
PAI_CERT="${ATT}/Chip-Test-PAI-FFF2-NoPID-Cert.pem"
CD_FILE="${CD}/Chip-Test-CD-FFF2-8002.der"

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage_count "$(basename "$0")"
    exit 0
fi

parse_args "$@"
run_mfg

#!/usr/bin/env bash
# Flash release firmware + per-unit factory data onto one board.
#
# Usage:
#   ./flash_unit.sh <app_dir> <mfg_unit_dir> [serial_port]
#
# Example:
#   ./flash_unit.sh iotBasicBinarySensor \
#     out/mfg/beta-batch-24-07-2026/fff2_8001/<uuid> /dev/ttyACM0
set -euo pipefail

APP_DIR="${1:?app dir (e.g. iotBasicBinarySensor)}"
MFG_UNIT_DIR="${2:?mfg output dir for this unit (contains *-partition.bin or *_fctry.bin)}"
PORT="${3:-${ESPPORT:-}}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD="${REPO_ROOT}/${APP_DIR}/build"

if [[ -z "${PORT}" ]]; then
    echo "error: pass serial port as third argument or set ESPPORT" >&2
    exit 1
fi

if [[ ! -d "${BUILD}" ]]; then
    echo "error: build dir not found: ${BUILD} — run idf.py build in ${APP_DIR} first" >&2
    exit 1
fi

FCTRY="$(find "${MFG_UNIT_DIR}" -maxdepth 1 \( -name '*-partition.bin' -o -name '*_fctry.bin' \) | head -1)"
SEC_CERT="$(find "${MFG_UNIT_DIR}" -maxdepth 1 -name '*_esp_secure_cert.bin' | head -1)"

if [[ -z "${FCTRY}" ]]; then
    echo "error: no *-partition.bin or *_fctry.bin under ${MFG_UNIT_DIR}" >&2
    exit 1
fi

echo "==> Flashing ${APP_DIR} firmware + factory data on ${PORT}"

FLASH_ARGS=(
    --chip auto
    --port "${PORT}"
    --baud 921600
    write_flash
    0x0 "${BUILD}/bootloader/bootloader.bin"
    0xc000 "${BUILD}/partition_table/partition-table.bin"
    0x20000 "${BUILD}/${APP_DIR}.bin"
)

if [[ -n "${SEC_CERT}" ]]; then
    FLASH_ARGS+=(0xd000 "${SEC_CERT}")
fi

FLASH_ARGS+=(0x3E0000 "${FCTRY}")

python -m esptool "${FLASH_ARGS[@]}"

echo "==> Flash complete. Label this board with the QR from the matching summary CSV row."

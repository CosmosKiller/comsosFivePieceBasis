#!/usr/bin/env bash
# Shared helpers for esp-matter-mfg-tool (sourced by SKU scripts).
set -euo pipefail

# Matter SDK test credential tree (connectedhomeip inside esp-matter).
# Prefer ESP_MATTER_PATH — a stale MATTER_SDK_PATH in the shell (e.g. from
# idf/matter export) often points at a broken relative path.
if [[ -n "${ESP_MATTER_PATH:-}" ]]; then
    if [[ "${ESP_MATTER_PATH}" == */path/to/esp-matter* ]]; then
        echo "error: ESP_MATTER_PATH is still the docs placeholder (/path/to/esp-matter)" >&2
        echo "  set your real esp-matter clone, e.g.: export ESP_MATTER_PATH=\$HOME/esp/esp-matter" >&2
        exit 1
    fi
    RESOLVED_MATTER_SDK="${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip"
    if [[ -n "${MATTER_SDK_PATH:-}" && "${MATTER_SDK_PATH}" != "${RESOLVED_MATTER_SDK}" ]]; then
        echo "note: ignoring MATTER_SDK_PATH=${MATTER_SDK_PATH}" >&2
        echo "      using ESP_MATTER_PATH -> ${RESOLVED_MATTER_SDK}" >&2
    fi
    MATTER_SDK_PATH="${RESOLVED_MATTER_SDK}"
elif [[ -n "${MATTER_SDK_PATH:-}" ]]; then
    if [[ "${MATTER_SDK_PATH}" == */path/to/* ]]; then
        echo "error: MATTER_SDK_PATH looks like a docs placeholder" >&2
        exit 1
    fi
else
    echo "error: set ESP_MATTER_PATH (recommended) or MATTER_SDK_PATH before running mfg scripts" >&2
    echo "  example: export ESP_MATTER_PATH=\$HOME/esp/esp-matter" >&2
    exit 1
fi

CRED="${MATTER_SDK_PATH}/credentials/test"
ATT="${CRED}/attestation"
CD="${CRED}/certification-declaration"

require_matter_creds() {
    local missing=0
    for f in "${PAI_KEY}" "${PAI_CERT}" "${CD_FILE}"; do
        if [[ ! -f "${f}" ]]; then
            echo "error: credential file not found: ${f}" >&2
            missing=1
        fi
    done
    if [[ "${missing}" -eq 1 ]]; then
        echo "  MATTER_SDK_PATH=${MATTER_SDK_PATH}" >&2
        echo "  ESP_MATTER_PATH=${ESP_MATTER_PATH:-<unset>}" >&2
        echo "  If ESP_MATTER_PATH is correct, run: unset MATTER_SDK_PATH" >&2
        exit 1
    fi
}

# Cosmos test vendor (Espressif Chip-Test-* creds — beta / lab only).
export COSMOS_VID="${COSMOS_VID:-0xFFF2}"
export COSMOS_VENDOR_NAME="${COSMOS_VENDOR_NAME:-Cosmos IoT}"

# Repo root (tools/mfg -> ../..)
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# Generated factory images, CSV manifests, QR payloads (gitignored via out/).
DEFAULT_OUTDIR="${REPO_ROOT}/out/mfg"

usage_count() {
    local script="$1"
    cat <<EOF
Usage: ${script} [count] [outdir]

  count   Number of unique factory sets to generate (default: 1).
  outdir  Output directory (default: ${DEFAULT_OUTDIR}/<sku>).

Environment:
  MATTER_SDK_PATH / ESP_MATTER_PATH   Matter SDK root
  BUNDLE_ID                           Optional serial prefix (e.g. BETA007)
EOF
}

require_mfg_tool() {
    if ! command -v esp-matter-mfg-tool >/dev/null 2>&1; then
        echo "error: esp-matter-mfg-tool not found. Install with:" >&2
        echo "  python3 -m pip install esp-matter-mfg-tool" >&2
        exit 1
    fi
}

parse_args() {
    COUNT="${1:-1}"
    OUTDIR="${2:-${DEFAULT_OUTDIR}/${SKU_KEY}}"
}

# Optional BUNDLE_ID prefix for --serial-num on single-unit runs.
serial_num_arg() {
    if [[ -n "${BUNDLE_ID:-}" && "${COUNT}" -eq 1 ]]; then
        echo "--serial-num" "${BUNDLE_ID}-${SKU_CODE}-001"
    fi
}

run_mfg() {
    require_mfg_tool
    require_matter_creds
    mkdir -p "${OUTDIR}"
    echo "==> ${SKU_NAME}: generating ${COUNT} factory set(s) -> ${OUTDIR}"
    esp-matter-mfg-tool -n "${COUNT}" \
        -cn "${CN_PREFIX}" \
        -v "${COSMOS_VID}" -p "${PID}" --pai \
        -k "${PAI_KEY}" -c "${PAI_CERT}" \
        -cd "${CD_FILE}" \
        --vendor-name "${COSMOS_VENDOR_NAME}" \
        --product-name "${PRODUCT_NAME}" \
        --hw-ver "${HW_VER}" --hw-ver-str "${HW_VER_STR}" \
        --dac-in-secure-cert \
        --target "${IDF_TARGET}" \
        --outdir "${OUTDIR}" \
        $(serial_num_arg)
    echo "==> Done. See summary-*.csv under ${OUTDIR} for QR codes and manual pairing codes."
}

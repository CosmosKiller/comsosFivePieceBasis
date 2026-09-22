#!/usr/bin/env bash
# Build all Matter firmware apps (see docs/BUILD.md for toolchain pins).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

# app:target
APPS=(
    "iotDoorSensor:esp32c6"
    "iotDualModeBtn:esp32c6"
    "iotEnvironmentalSensor:esp32c5"
    "iotBedsideLamp:esp32c6"
    "iotSecurityCamera:esp32s3"
)

if [[ -z "${ESP_MATTER_PATH:-}" ]]; then
    echo "error: ESP_MATTER_PATH is not set" >&2
    exit 1
fi

if [[ -z "${IDF_PATH:-}" ]]; then
    echo "error: IDF_PATH is not set (source \$IDF_PATH/export.sh)" >&2
    exit 1
fi

FRESH_CONFIG="${FRESH_CONFIG:-0}"

for entry in "${APPS[@]}"; do
    app="${entry%%:*}"
    target="${entry##*:}"
    echo "=== build: $app (target=$target) ==="
    cd "$ROOT/$app"

    if [[ "$FRESH_CONFIG" == "1" ]]; then
        rm -f sdkconfig sdkconfig.old
    fi

    if [[ ! -f sdkconfig ]] || ! grep -q "CONFIG_IDF_TARGET=\"${target}\"" sdkconfig 2>/dev/null; then
        PREVIEW=()
        if [[ "$target" == "esp32p4" || "$target" == "esp32c5" ]]; then
            PREVIEW=(--preview)
        fi
        idf.py "${PREVIEW[@]}" set-target "$target"
    fi

    PREVIEW=()
    if [[ "$target" == "esp32p4" || "$target" == "esp32c5" ]]; then
        PREVIEW=(--preview)
    fi
    idf.py "${PREVIEW[@]}" build
done

echo "Note: SKU5 (iotDoorIntercom) is dual-image — build with:"
echo "  COSMOS_FIVE_PIECE_PATH=$ROOT iotDoorIntercom/scripts/build_c6.sh"
echo "  COSMOS_FIVE_PIECE_PATH=$ROOT iotDoorIntercom/scripts/build_p4.sh"
echo "All listed apps built successfully."

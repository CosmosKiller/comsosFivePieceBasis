#!/usr/bin/env bash
# Build CosmOS SKU5 P4 media adapter (streaming_only + monorepo p4 I/O).
set -euo pipefail

: "${KVS_SDK_PATH:?set KVS_SDK_PATH}"
: "${IDF_PATH:?source ESP-IDF export.sh first}"

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
export COSMOS_FIVE_PIECE_PATH="${COSMOS_FIVE_PIECE_PATH:-$REPO_ROOT}"

cd "$KVS_SDK_PATH/examples/streaming_only"
# Function EV v1.6 BSP (ES8311 audio) + Waveshare console/flash/cam overlay.
# Order matches docs/BUILD.md SKU5 — do not drop the v1.6 layer (selects
# CONFIG_BSP_SELECT_ESP32_P4_FUNCTION_EV_BOARD + disables video player).
DEFAULTS="sdkconfig.defaults"
if [[ -f sdkconfig.defaults.esp32p4 ]]; then
  DEFAULTS="$DEFAULTS;sdkconfig.defaults.esp32p4"
fi
if [[ -f sdkconfig.defaults.p4_function_ev_board_v16.esp32p4 ]]; then
  DEFAULTS="$DEFAULTS;sdkconfig.defaults.p4_function_ev_board_v16.esp32p4"
fi
if [[ -f sdkconfig.defaults.waveshare_p4_wifi6.esp32p4 ]]; then
  DEFAULTS="$DEFAULTS;sdkconfig.defaults.waveshare_p4_wifi6.esp32p4"
fi
idf.py -D SDKCONFIG_DEFAULTS="$DEFAULTS" set-target esp32p4
idf.py build
echo "P4 artifact: $PWD/build/streaming_only.bin"
echo "Defaults chain: $DEFAULTS"

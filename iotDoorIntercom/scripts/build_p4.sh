#!/usr/bin/env bash
# Build CosmOS SKU5 P4 media adapter (streaming_only + monorepo p4 I/O).
set -euo pipefail

: "${KVS_SDK_PATH:?set KVS_SDK_PATH}"
: "${IDF_PATH:?source ESP-IDF export.sh first}"

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
export COSMOS_FIVE_PIECE_PATH="${COSMOS_FIVE_PIECE_PATH:-$REPO_ROOT}"

cd "$KVS_SDK_PATH/examples/streaming_only"
# Waveshare P4-WIFI6 console overlay when present
DEFAULTS="sdkconfig.defaults"
if [[ -f sdkconfig.defaults.esp32p4 ]]; then
  DEFAULTS="$DEFAULTS;sdkconfig.defaults.esp32p4"
fi
if [[ -f sdkconfig.defaults.waveshare_p4_wifi6.esp32p4 ]]; then
  DEFAULTS="$DEFAULTS;sdkconfig.defaults.waveshare_p4_wifi6.esp32p4"
fi
idf.py -D SDKCONFIG_DEFAULTS="$DEFAULTS" set-target esp32p4
idf.py build
echo "P4 artifact: $PWD/build/streaming_only.bin"

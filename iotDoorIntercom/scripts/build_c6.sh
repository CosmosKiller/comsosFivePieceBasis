#!/usr/bin/env bash
# Build CosmOS SKU5 C6 Matter camera (split_mode + monorepo security_endpoints).
set -euo pipefail

: "${ESP_MATTER_PATH:?set ESP_MATTER_PATH}"
: "${IDF_PATH:?source ESP-IDF export.sh first}"

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
export COSMOS_FIVE_PIECE_PATH="${COSMOS_FIVE_PIECE_PATH:-$REPO_ROOT}"
export ESP_MATTER_DEVICE_PATH="${ESP_MATTER_DEVICE_PATH:-$ESP_MATTER_PATH/device_hal/device/esp32c6_devkit_c}"
# Required for CHIP gn / pigweed when cmake reconfigures
export _PW_ACTUAL_ENVIRONMENT_ROOT="${_PW_ACTUAL_ENVIRONMENT_ROOT:-$ESP_MATTER_PATH/connectedhomeip/connectedhomeip/.environment}"
export PATH="${_PW_ACTUAL_ENVIRONMENT_ROOT}/cipd/packages/pigweed:${PATH}"

cd "$ESP_MATTER_PATH/examples/camera/split_mode"
idf.py set-target esp32c6
idf.py build
echo "C6 artifact: $PWD/build/camera.bin"

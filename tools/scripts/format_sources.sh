#!/usr/bin/env bash
# Format application C/C++ sources with the repo .clang-format (see docs/CODE_STYLE.md).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CLANG_FORMAT="${CLANG_FORMAT:-}"

if [[ -z "$CLANG_FORMAT" ]]; then
    for candidate in clang-format clang-format-18 clang-format-17 clang-format-16; do
        if command -v "$candidate" >/dev/null 2>&1; then
            CLANG_FORMAT="$candidate"
            break
        fi
    done
fi

if [[ -z "$CLANG_FORMAT" ]]; then
    echo "error: clang-format not found. Install it (e.g. apt install clang-format) or set CLANG_FORMAT." >&2
    exit 1
fi

APPS=(iotDoorSensor iotDualModeBtn iotEnvironmentalSensor iotBedsideLamp iotDoorIntercom)
COMPONENTS=(cosmos_battery cosmos_matter_common)
shopt -s nullglob

format_tree() {
    local path="$1"
    [[ -d "$path" ]] || return 0
    local file
    for file in "$path"/*.{c,cpp,h} "$path"/include/*.{c,cpp,h}; do
        [[ -f "$file" ]] || continue
        echo "format: $file"
        "$CLANG_FORMAT" -i "$file"
    done
}

for app in "${APPS[@]}"; do
    format_tree "$ROOT/$app/main"
    format_tree "$ROOT/$app/tasks"
done

for comp in "${COMPONENTS[@]}"; do
    format_tree "$ROOT/components/$comp"
    format_tree "$ROOT/components/$comp/include"
done

echo "done."

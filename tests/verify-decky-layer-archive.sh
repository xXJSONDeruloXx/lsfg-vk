#!/usr/bin/env bash
set -euo pipefail

archive_path=${1:?usage: verify-decky-layer-archive.sh <archive> <x86_64|aarch64>}
architecture=${2:?usage: verify-decky-layer-archive.sh <archive> <x86_64|aarch64>}

if [[ ! -f "$archive_path" ]]; then
    echo "missing archive: $archive_path" >&2
    exit 1
fi

work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT

tar -xJf "$archive_path" -C "$work_dir"
lib="$work_dir/liblsfg-vk-layer.so"
manifest="$work_dir/VkLayer_LSFGVK_frame_generation.json"

[[ -f "$lib" ]] || { echo "archive is missing layer library" >&2; exit 1; }
[[ -f "$manifest" ]] || { echo "archive is missing layer manifest" >&2; exit 1; }

case "$architecture" in
    x86_64) file "$lib" | grep -q 'x86-64' ;;
    aarch64) file "$lib" | grep -q 'ARM aarch64' ;;
    *) echo "unknown architecture: $architecture" >&2; exit 1 ;;
esac

python3 - "$manifest" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as manifest_file:
    manifest = json.load(manifest_file)
layer = manifest["layer"]
assert layer["name"] == "VK_LAYER_LSFGVK_frame_generation"
assert layer["library_path"] == "liblsfg-vk-layer.so"
assert layer["disable_environment"] == {"DISABLE_LSFGVK": "1"}
PY

if ! readelf -d "$lib" | grep -q 'Shared library: \[libstdc++'; then
    echo "layer must dynamically link libstdc++ for compatibility with Steam runtimes" >&2
    exit 1
fi

if ! readelf -d "$lib" | grep -q 'Shared library: \[libgcc_s'; then
    echo "layer must dynamically link libgcc_s for compatibility with Steam runtimes" >&2
    exit 1
fi

echo "verified $archive_path ($architecture)"

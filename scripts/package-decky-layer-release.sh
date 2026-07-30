#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUNDLE_ROOT="${BUNDLE_ROOT:-$ROOT_DIR/out/layer-bundles}"
OUTPUT_DIR="${OUTPUT_DIR:-$ROOT_DIR/out/release}"

usage() {
    cat <<'EOF'
Usage: scripts/package-decky-layer-release.sh [--bundle-root <path>] [--output-dir <path>]

Packages the glibc x86_64 and aarch64 bundles produced by
scripts/build-layer-bundles.sh into the two immutable Decky release assets:
  lsfg-vk-layer-x86_64.tar.xz
  lsfg-vk-layer-aarch64.tar.xz
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --bundle-root) BUNDLE_ROOT="$2"; shift 2 ;;
        --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done

package_bundle() {
    local target="$1"
    local output_name="$2"
    local bundle_dir="$BUNDLE_ROOT/$target"
    local output_path="$OUTPUT_DIR/$output_name"
    local library="$bundle_dir/liblsfg-vk-layer.so"
    local manifest="$bundle_dir/VkLayer_LSFGVK_frame_generation.json"

    [[ -f "$library" ]] || { echo "missing library: $library" >&2; exit 1; }
    [[ -f "$manifest" ]] || { echo "missing manifest: $manifest" >&2; exit 1; }

    tar --sort=name --owner=0 --group=0 --numeric-owner \
        -C "$bundle_dir" -cJf "$output_path" \
        liblsfg-vk-layer.so VkLayer_LSFGVK_frame_generation.json
    echo "created $output_path"
}

mkdir -p "$OUTPUT_DIR"
package_bundle glibc-x86_64 lsfg-vk-layer-x86_64.tar.xz
package_bundle glibc-aarch64 lsfg-vk-layer-aarch64.tar.xz

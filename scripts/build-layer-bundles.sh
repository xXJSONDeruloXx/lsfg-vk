#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-$ROOT_DIR/out/build}"
OUT_ROOT="${OUT_ROOT:-$ROOT_DIR/out/layer-bundles}"
HOST_CXX="${HOST_CXX:-}"
GLIBC_AARCH64_CXX="${GLIBC_AARCH64_CXX:-}"
ANDROID_API="${ANDROID_API:-26}"
ANDROID_NDK_HOME="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-${ANDROID_NDK:-}}}"
VULKAN_HEADERS_DIR="${VULKAN_HEADERS_DIR:-}"
VULKAN_HEADERS_REF="${VULKAN_HEADERS_REF:-vulkan-sdk-1.4.328}"
CLEAN=0

usage() {
    echo "Build portable lsfg-vk layer bundles for glibc and/or Android bionic."
    echo
    echo "Usage: $(basename "$0") [options]"
    echo
    echo "Targets:"
    echo "  glibc-x86_64"
    echo "  glibc-aarch64"
    echo "  android-arm64-v8a"
    echo "  android-x86_64"
    echo
    echo "Options:"
    echo "  --all                     Build all targets (default)"
    echo "  --target <name>           Build a specific target; may be passed multiple times"
    echo "  --glibc                   Shortcut for --target glibc-x86_64"
    echo "  --glibc-aarch64           Shortcut for --target glibc-aarch64"
    echo "  --android-arm64           Shortcut for --target android-arm64-v8a"
    echo "  --android-x86_64          Shortcut for --target android-x86_64"
    echo "  --android-ndk <path>      Path to the Android NDK"
    echo "  --android-api <level>     Android API level to target (default: ${ANDROID_API})"
    echo "  --vulkan-headers-dir <p>  Path to a Vulkan-Headers include dir or repo checkout"
    echo "  --host-cxx <compiler>     Host C++ compiler for glibc x86_64 builds"
    echo "  --glibc-aarch64-cxx <c>   Cross C++ compiler for glibc-aarch64 builds"
    echo "  --build-root <path>       Build directory root (default: ${BUILD_ROOT})"
    echo "  --out-dir <path>          Output bundle root (default: ${OUT_ROOT})"
    echo "  --clean                   Remove each target's existing build/output directories first"
    echo "  -h, --help                Show this help"
    echo
    echo "Notes:"
    echo "  - Android ABIs are arm64-v8a and x86_64. arm64ec is a Windows ABI and is not supported here."
    echo "  - glibc-aarch64 expects a Linux glibc cross compiler such as aarch64-linux-gnu-g++."
    echo "  - The script builds the Vulkan layer bundle only (.so + manifest), not the CLI/UI."
}

ensure_host_cxx() {
    if [[ -n "$HOST_CXX" ]]; then
        return
    fi

    if command -v clang++ >/dev/null 2>&1; then
        HOST_CXX="clang++"
        return
    fi
    if command -v c++ >/dev/null 2>&1; then
        HOST_CXX="c++"
        return
    fi

    echo "error: no host C++ compiler found; set --host-cxx or HOST_CXX" >&2
    exit 1
}

ensure_glibc_aarch64_cxx() {
    if [[ -n "$GLIBC_AARCH64_CXX" ]]; then
        return
    fi

    if command -v aarch64-linux-gnu-g++ >/dev/null 2>&1; then
        GLIBC_AARCH64_CXX="aarch64-linux-gnu-g++"
        return
    fi

    echo "error: no glibc aarch64 cross compiler found; set --glibc-aarch64-cxx or GLIBC_AARCH64_CXX" >&2
    exit 1
}

ensure_android_ndk() {
    if [[ -z "$ANDROID_NDK_HOME" ]]; then
        echo "error: Android NDK not found; set --android-ndk or ANDROID_NDK_HOME" >&2
        exit 1
    fi
    if [[ ! -f "$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" ]]; then
        echo "error: invalid Android NDK path: $ANDROID_NDK_HOME" >&2
        exit 1
    fi
}

ensure_vulkan_headers() {
    if [[ -n "$VULKAN_HEADERS_DIR" ]]; then
        if [[ -f "$VULKAN_HEADERS_DIR/vulkan/vk_layer.h" ]]; then
            return
        fi
        if [[ -f "$VULKAN_HEADERS_DIR/include/vulkan/vk_layer.h" ]]; then
            VULKAN_HEADERS_DIR="$VULKAN_HEADERS_DIR/include"
            return
        fi

        echo "error: Vulkan headers not found under: $VULKAN_HEADERS_DIR" >&2
        echo "expected vulkan/vk_layer.h or include/vulkan/vk_layer.h" >&2
        exit 1
    fi

    local checkout_dir="$ROOT_DIR/.build-deps/Vulkan-Headers"
    if [[ ! -f "$checkout_dir/include/vulkan/vk_layer.h" ]]; then
        mkdir -p "$(dirname "$checkout_dir")"
        git clone --depth 1 --branch "$VULKAN_HEADERS_REF" \
            https://github.com/KhronosGroup/Vulkan-Headers "$checkout_dir"
    fi

    VULKAN_HEADERS_DIR="$checkout_dir/include"
}

copy_bundle() {
    local install_dir="$1"
    local bundle_dir="$2"
    local lib_path="$install_dir/lib/liblsfg-vk-layer.so"
    local manifest_path="$install_dir/share/vulkan/implicit_layer.d/VkLayer_LSFGVK_frame_generation.json"

    if [[ ! -f "$lib_path" ]]; then
        echo "error: built layer not found at $lib_path" >&2
        exit 1
    fi
    if [[ ! -f "$manifest_path" ]]; then
        echo "error: built layer manifest not found at $manifest_path" >&2
        exit 1
    fi

    mkdir -p "$bundle_dir"
    cp "$lib_path" "$bundle_dir/"
    cp "$manifest_path" "$bundle_dir/"
}

build_target() {
    local target="$1"
    local build_dir="$BUILD_ROOT/$target"
    local install_dir="$BUILD_ROOT/install/$target"
    local bundle_dir="$OUT_ROOT/$target"

    if [[ "$CLEAN" -eq 1 ]]; then
        rm -rf "$build_dir" "$install_dir" "$bundle_dir"
    fi

    mkdir -p "$BUILD_ROOT" "$OUT_ROOT"

    local -a cmake_args=(
        -S "$ROOT_DIR"
        -B "$build_dir"
        -G Ninja
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX="$install_dir"
        -DLSFGVK_BUILD_VK_LAYER=ON
        -DLSFGVK_BUILD_UI=OFF
        -DLSFGVK_BUILD_CLI=OFF
        -DLSFGVK_LAYER_LIBRARY_PATH=liblsfg-vk-layer.so
        -DLSFGVK_VULKAN_HEADERS_DIR="$VULKAN_HEADERS_DIR"
    )

    case "$target" in
        glibc-x86_64)
            ensure_host_cxx
            cmake_args+=(
                -DCMAKE_CXX_COMPILER="$HOST_CXX"
            )
            ;;
        glibc-aarch64)
            ensure_glibc_aarch64_cxx
            cmake_args+=(
                -DCMAKE_SYSTEM_NAME=Linux
                -DCMAKE_SYSTEM_PROCESSOR=aarch64
                -DCMAKE_CXX_COMPILER="$GLIBC_AARCH64_CXX"
            )
            ;;
        android-arm64-v8a)
            ensure_android_ndk
            cmake_args+=(
                -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake"
                -DANDROID_ABI=arm64-v8a
                -DANDROID_PLATFORM="$ANDROID_API"
                -DANDROID_STL=c++_static
            )
            ;;
        android-x86_64)
            ensure_android_ndk
            cmake_args+=(
                -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake"
                -DANDROID_ABI=x86_64
                -DANDROID_PLATFORM="$ANDROID_API"
                -DANDROID_STL=c++_static
            )
            ;;
        *)
            echo "error: unknown target: $target" >&2
            exit 1
            ;;
    esac

    echo ">>> Configuring $target"
    cmake "${cmake_args[@]}"

    echo ">>> Building $target"
    cmake --build "$build_dir" --parallel --target lsfg-vk-layer

    echo ">>> Installing $target"
    cmake --install "$build_dir"

    echo ">>> Bundling $target"
    copy_bundle "$install_dir" "$bundle_dir"
}

declare -a targets=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        --all)
            targets=(glibc-x86_64 glibc-aarch64 android-arm64-v8a android-x86_64)
            shift
            ;;
        --target)
            targets+=("$2")
            shift 2
            ;;
        --glibc)
            targets+=("glibc-x86_64")
            shift
            ;;
        --glibc-aarch64)
            targets+=("glibc-aarch64")
            shift
            ;;
        --android-arm64)
            targets+=("android-arm64-v8a")
            shift
            ;;
        --android-x86_64)
            targets+=("android-x86_64")
            shift
            ;;
        --android-ndk)
            ANDROID_NDK_HOME="$2"
            shift 2
            ;;
        --android-api)
            ANDROID_API="$2"
            shift 2
            ;;
        --vulkan-headers-dir)
            VULKAN_HEADERS_DIR="$2"
            shift 2
            ;;
        --host-cxx)
            HOST_CXX="$2"
            shift 2
            ;;
        --glibc-aarch64-cxx)
            GLIBC_AARCH64_CXX="$2"
            shift 2
            ;;
        --build-root)
            BUILD_ROOT="$2"
            shift 2
            ;;
        --out-dir)
            OUT_ROOT="$2"
            shift 2
            ;;
        --clean)
            CLEAN=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "error: unknown argument: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

if [[ ${#targets[@]} -eq 0 ]]; then
    targets=(glibc-x86_64 glibc-aarch64 android-arm64-v8a android-x86_64)
fi

ensure_vulkan_headers

# de-duplicate while preserving order
seen=":"
declare -a unique_targets=()
for target in "${targets[@]}"; do
    if [[ "$seen" == *":$target:"* ]]; then
        continue
    fi
    seen+="$target:"
    unique_targets+=("$target")
done

for target in "${unique_targets[@]}"; do
    build_target "$target"
done

echo
echo "Built layer bundles:"
for target in "${unique_targets[@]}"; do
    echo "  $OUT_ROOT/$target"
done

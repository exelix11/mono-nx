#!/bin/bash

# This script collects the files needed to build the interpreter and AOT binaries without having to manually build mono
# Paths are chosen empirically from build scripts and may break at any time
# Also i don't think this will work across dkp toolchain updates so you should use the pinned version in the dockerfile and hope for the best

set -euo pipefail

ROOT_DIR=$(realpath .)
SDK_STAGE="$ROOT_DIR/sdk_build"
SDK_ZIP="$ROOT_DIR/mono-nx-sdk-linux-x64.zip"

require_env() {
    local name="$1"

    if [ -z "${!name:-}" ]; then
        echo "Missing required environment variable: $name" >&2
        exit 1
    fi
}

require_dir() {
    local path="$1"

    if [ ! -d "$path" ]; then
        echo "Missing required directory: $path" >&2
        exit 1
    fi
}

copy_dir() {
    local src="$1"
    local dest="$2"

    require_dir "$src"
    mkdir -p "$dest"
    cp -a "$src"/. "$dest"/
}

copy_file() {
    local src="$1"
    local dest="$2"

    if [ ! -f "$src" ]; then
        echo "Missing required file: $src" >&2
        exit 1
    fi

    mkdir -p "$(dirname "$dest")"
    cp "$src" "$dest"
}

copy_dlls() {
    local src_dir="$1"
    local dest_dir="$2"
    local -a dlls=()

    require_dir "$src_dir"
    mkdir -p "$dest_dir"

    shopt -s nullglob
    dlls=("$src_dir"/*.dll)
    shopt -u nullglob

    if [ "${#dlls[@]}" -eq 0 ]; then
        echo "No DLLs found in: $src_dir" >&2
        exit 1
    fi

    cp "${dlls[@]}" "$dest_dir"/
}

copy_static_libs() {
    local src_dir="$1"
    local dest_dir="$2"
    local -a archives=()

    require_dir "$src_dir"
    mkdir -p "$dest_dir"

    shopt -s nullglob
    archives=("$src_dir"/*.a)
    shopt -u nullglob

    if [ "${#archives[@]}" -eq 0 ]; then
        echo "No static libraries found in: $src_dir" >&2
        exit 1
    fi

    cp "${archives[@]}" "$dest_dir"/
}

require_env ICU_NX_INSTALL_DIR
require_env MONO_NX_ROOT

ICU_INCLUDE_DIR="$ICU_NX_INSTALL_DIR/include"
ICU_LIB_DIR="$ICU_NX_INSTALL_DIR/lib"
ICU_DATA_FILE="$ICU_NX_INSTALL_DIR/share/icu/77.1/icudt77l.dat"

MONO_DLL_DIR="$MONO_NX_ROOT/artifacts/bin/mono/libnx.arm64.Debug"
MONO_INCLUDE_DIR="$MONO_DLL_DIR/include/mono-2.0"
RUNTIME_DLL_DIR="$MONO_NX_ROOT/artifacts/bin/runtime/net9.0-libnx-Debug-arm64"
NATIVE_LIB_DIR="$MONO_NX_ROOT/artifacts/bin/native/net9.0-libnx-Debug-arm64"
MONO_LIB_DIR="$MONO_NX_ROOT/artifacts/obj/mono/libnx.arm64.Debug/out/lib"
ZLIB_DIR="$MONO_NX_ROOT/artifacts/obj/mono/libnx.arm64.Debug/_deps/fetchzlibng-build"
LINKER_DIR="$MONO_NX_ROOT/artifacts/bin/Mono.Linker/Debug/net9.0"
AOT_CROSS_DIR="$MONO_NX_ROOT/artifacts/bin/mono/linux.x64.Debug/cross/linux-x64/libnx-arm64"
ILLINK_DIR="$MONO_NX_ROOT/src/mono/System.Private.CoreLib/src/ILLink"

ILLINK_FILES=(
    ILLink.Descriptors.xml
    ILLink.LinkAttributes.xml
)

echo "Preparing SDK staging tree..."
rm -rf "$SDK_STAGE"
mkdir -p "$SDK_STAGE"
rm -f "$SDK_ZIP"

echo "Collecting ICU payload..."
copy_dir "$ICU_INCLUDE_DIR" "$SDK_STAGE/icu/libnx/include"
copy_static_libs "$ICU_LIB_DIR" "$SDK_STAGE/icu/libnx/lib"

copy_file "$ICU_DATA_FILE" "$SDK_STAGE/icu/libnx/share/icu/77.1/icudt77l.dat"

echo "Collecting Mono runtime payload..."
copy_dir "$MONO_INCLUDE_DIR" "$SDK_STAGE/dotnet_runtime/artifacts/bin/mono/libnx.arm64.Debug/include/mono-2.0"
copy_dlls "$MONO_DLL_DIR" "$SDK_STAGE/dotnet_runtime/artifacts/bin/mono/libnx.arm64.Debug"
copy_dlls "$RUNTIME_DLL_DIR" "$SDK_STAGE/dotnet_runtime/artifacts/bin/runtime/net9.0-libnx-Debug-arm64"
copy_dir "$LINKER_DIR" "$SDK_STAGE/dotnet_runtime/artifacts/bin/Mono.Linker/Debug/net9.0"
copy_dir "$AOT_CROSS_DIR" "$SDK_STAGE/dotnet_runtime/artifacts/bin/mono/linux.x64.Debug/cross/linux-x64/libnx-arm64"
copy_static_libs "$NATIVE_LIB_DIR" "$SDK_STAGE/dotnet_runtime/artifacts/bin/native/net9.0-libnx-Debug-arm64"
copy_static_libs "$MONO_LIB_DIR" "$SDK_STAGE/dotnet_runtime/artifacts/obj/mono/libnx.arm64.Debug/out/lib"
copy_static_libs "$ZLIB_DIR" "$SDK_STAGE/dotnet_runtime/artifacts/obj/mono/libnx.arm64.Debug/_deps/fetchzlibng-build"

echo "Collecting ILLink configuration..."
for file_name in "${ILLINK_FILES[@]}"; do
    copy_file "$ILLINK_DIR/$file_name" "$SDK_STAGE/dotnet_runtime/src/mono/System.Private.CoreLib/src/ILLink/$file_name"
done

echo "Creating SDK archive..."
(
    cd "$SDK_STAGE"
    zip -r -9 "$SDK_ZIP" icu dotnet_runtime >/dev/null
)

echo Done
#!/bin/sh

# Run with source env.sh

if [ -z $DEVKITPRO ]; then
    echo "DEVKITPRO Directory not found"
    return
fi

mkdir -p icu/libnx
export ICU_NX_INSTALL_DIR=$(realpath icu/libnx)

export MONO_ROOT=$(realpath .)/dotnet_runtime
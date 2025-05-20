#!/bin/sh

set -e

if [ -z $DEVKITPRO ]; then
    echo "DEVKITPRO Directory not found"
    exit 1
fi

if [ ! -f $ICU_NX_INSTALL_DIR/lib/libicudata.a ]; then
    echo "Run env.sh and build libicu first"
    exit 1
fi

if [ ! -d $MONO_ROOT ]; then
    echo "The mono root is missing, did you clone the repo?"
    exit 1
fi

cd $MONO_ROOT

echo building mono interpreter

export ROOTFS_DIR=$DEVKITPRO
./build.sh --subset mono.runtime+mono.corelib+libs.native+libs.sfx --cross -a arm64 --os libnx

echo building target AOT offsets

./build.sh -s mono.aotcross /p:MonoGenerateOffsetsOSGroups=libnx

echo building host AOT cross-compiler

# Force the build system to use the host toolchain
export ROOTFS_DIR=
./build.sh -s mono /p:AotHostArchitecture=x64 /p:AotHostOS=linux /p:MonoCrossAOTTargetOS=libnx /p:SkipMonoCrossJitConfigure=true /p:BuildMonoAOTCrossCompilerOnly=true /p:BuildMonoAOTCrossCompiler=true

# If everything went well, copy the output libraries to the sd output folder
cd ..

# This just contains the corlib
mkdir -p sd_files/mono/lib_net9.0
cp $MONO_ROOT/artifacts/bin/mono/libnx.arm64.Debug/*.dll sd_files/mono/lib_net9.0/

# This contains the rest of the framework
mkdir -p sd_files/mono/framework_net9.0
cp $MONO_ROOT/artifacts/bin/runtime/net9.0-libnx-Debug-arm64/*.dll sd_files/mono/framework_net9.0/

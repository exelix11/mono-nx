#!/bin/sh

# This script builds libicu for switch see reference: https://unicode-org.github.io/icu/userguide/icu4c/build.html#how-to-cross-compile-icu

set -e 

if [ ! -d $ICU_NX_INSTALL_DIR ]; then
    echo "Run the env.sh script in the repo root first"
    exit 1
fi

wget https://github.com/unicode-org/icu/releases/download/release-77-1/icu4c-77_1-src.tgz

tar -xvf icu4c-77_1-src.tgz

# First build for the host
mkdir host_build
cd host_build

sh ../icu/source/configure
make -j8

export ICU_HOST_DIR=$(pwd)

# Then cross build for switch
cd ..

# Apply the patch needed to build on switch
patch -p1 < libnx.patch

# Make the build folder
mkdir nx_build
cd nx_build

export CC=$DEVKITPRO/devkitA64/bin/aarch64-none-elf-gcc
export CXX=$DEVKITPRO/devkitA64/bin/aarch64-none-elf-g++

export ARCHFLAGS="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE"
export CFLAGS="$ARCHFLAGS -g -O2 -ffunction-sections"
export CXXFLAGS="$CFLAGS -fno-exceptions"

export LDFLAGS="-specs=$DEVKITPRO/libnx/switch.specs -g $ARCHFLAGS"

sh ../icu/source/configure --disable-renaming --disable-shared --enable-static --disable-dyload --with-data-packaging=archive -with-cross-build=$ICU_HOST_DIR -host=aarch64-pc-linux-gnu --disable-icuio --disable-extras --disable-tests --disable-samples --disable-tools --prefix=$ICU_NX_INSTALL_DIR

make -j8
make install

# Copy the data file to the sd card files
cp ./libnx/share/icu/77.1/icudt77l.dat ../sd_files/mono/etc
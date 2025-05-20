#!/bin/sh

set -e 

# Copy the icu data file to the sd card files
mkdir -p ../sd_files/mono/etc/
cp $ICU_NX_INSTALL_DIR/share/icu/77.1/icudt77l.dat  sd_files/mono/etc/

# Copy mono and the fallback dll
cp native/interpreter/mono_nx.nro sd_files/mono/
cp managed/pad_input/bin/Debug/net9.0/pad_input.dll sd_files/mono/

# Copy the managed binaries to the switch folder
mkdir -p sd_files/switch/
cp managed/pad_input/bin/Debug/net9.0/pad_input.dll sd_files/switch/
cp managed/example/bin/Debug/net9.0/example.dll sd_files/switch/

mkdir -p sd_files/switch/explorer_demo/
cp managed/explorer_demo/bin/Debug/net9.0/explorer_demo.dll sd_files/switch/explorer_demo/
cp managed/explorer_demo/bin/Debug/net9.0/OpenSans-Regular.ttf sd_files/switch/explorer_demo/

# Copy the aot demo if it has been built

if [ -f native/aot/aot_example.nro ]; then
    cp native/aot/aot_example.nro sd_files/switch/
fi
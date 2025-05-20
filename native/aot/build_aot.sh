#!/bin/sh

set -e

if [ -d output ]; then
    rm -rf output/
fi


if [ ! -e dotnet ]; then
    # docker
    if [ -e ~/.dotnet/dotnet ]; then
        export DOTNET_ROOT=~/.dotnet
        export PATH=$PATH:$DOTNET_ROOT
    else
        echo "dotnet was not found"
        exit 1
    fi
fi

echo Building the project...
dotnet build managed/program.csproj 

echo Trimming the assemblies...

ILLINK=$MONO_ROOT/artifacts/bin/Mono.Linker/Debug/net9.0/illink.dll
ILLINK_CFG=$MONO_ROOT/src/mono/System.Private.CoreLib/src/ILLink/ILLink.Descriptors.xml
ILLINK_CFG1=$MONO_ROOT/src/mono/System.Private.CoreLib/src/ILLink/ILLink.LinkAttributes.xml

LIB_ROOT=$BUILT_SD_ROOT/lib_net9.0/
FRAMEWORK_ROOT=$BUILT_SD_ROOT/framework_net9.0/

dotnet $ILLINK -x $ILLINK_CFG -x $ILLINK_CFG1 --feature System.Resources.UseSystemResourceKeys true -d $LIB_ROOT -d $FRAMEWORK_ROOT --trim-mode link -a managed/bin/Debug/net9.0/program.dll

echo Mono AOT build...

MONO_COMPILER=$MONO_ROOT/artifacts/bin/mono/linux.x64.Debug/cross/linux-x64/libnx-arm64/mono-aot-cross

export PATH=$PATH:$DEVKITPRO/devkitA64/bin/

echo "build log" > mono_aot.log

for file in output/*.dll; do
    $MONO_COMPILER --path=output/ --aot=full,static,tool-prefix=aarch64-none-elf- $file >> mono_aot.log
done

echo copying outputs
# Dlls are needed for metadata
cp output/*.dll romfs/
cp $ICU_NX_INSTALL_DIR/share/icu/77.1/icudt77l.dat romfs/

echo Static-linking symbols:
grep -r "Linking symbol:" mono_aot.log | sed "s/Linking symbol: '\([^']*\)'\./STATIC_MONO_SYM(\1);/"
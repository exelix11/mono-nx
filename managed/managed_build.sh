#!/bin/sh

set -e

if ! which dotnet >/dev/null 2>&1; then
    # docker
    if [ -e ~/.dotnet/dotnet ]; then
        export DOTNET_ROOT=~/.dotnet
        export PATH=$PATH:$DOTNET_ROOT
    else
        echo "dotnet was not found"
        exit 1
    fi
fi

dotnet build pad_input/pad_input.csproj
dotnet build example/example.csproj
dotnet build explorer_demo/explorer_demo.csproj
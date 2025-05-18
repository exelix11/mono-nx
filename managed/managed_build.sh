#!/bin/sh

set -e

dotnet build pad_input/pad_input.csproj
dotnet build example/example.csproj
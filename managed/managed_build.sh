#!/bin/sh

set -e

dotnet build hello_world/hello_world.csproj
dotnet build example/example.csproj
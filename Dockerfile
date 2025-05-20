FROM devkitpro/devkita64:latest

# This dockerfile can be used to build mono. It is based on the official devkita64 dockerfile with the additional build dependencies needed by dotnet

# Install script from https://github.com/dotnet/runtime/blob/main/.devcontainer/Dockerfile
# I removed cmake because we need to manually install a specific version
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
        cpio \
        build-essential \
        python3 \
        curl \
        git \
        lldb \
        llvm \
        liblldb-dev \
        libunwind8 \
        libunwind8-dev \
        gettext \
        libicu-dev \
        liblttng-ust-dev \
        libssl-dev \
        libkrb5-dev \
        ninja-build \
        tzdata 

# This image ships with cmake 3.18.4, the dotnet build system needs at least 3.20.0
# This specific version is the one i had in my vm, probably newer versions also work
RUN sudo apt purge cmake -y && \
    wget https://cmake.org/files/v3.22/cmake-3.22.1-linux-x86_64.sh && \
    chmod +x cmake-3.22.1-linux-x86_64.sh && \
    sudo mkdir -p /opt/cmake && \
    sudo ./cmake-3.22.1-linux-x86_64.sh --skip-license --prefix=/opt/cmake && \
    rm cmake-3.22.1-linux-x86_64.sh && \
    sudo ln -s /opt/cmake/bin/* /usr/local/bin/

# Install dotnet 9.0 which is not present in the apt repositories for this distro
RUN wget https://dot.net/v1/dotnet-install.sh -O dotnet-install.sh && \
    chmod +x dotnet-install.sh && \
    ./dotnet-install.sh --channel 9.0 && \
    rm dotnet-install.sh

# More dependencies not present in the dka64 image
RUN sudo apt install -y clang libclang-dev

# Needed by the offset_tool script
RUN sudo ln -sf "$(find /usr/lib -name 'libclang.so' -print -quit)" /usr/local/lib/libclang.so

# This must be mounted from the host
WORKDIR /mono-nx

ENTRYPOINT [ "/bin/bash", "-c", "source /mono-nx/env.sh && /bin/bash" ]
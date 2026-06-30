#!/bin/bash
# Install dependencies on Debian / Ubuntu / Termux PRoot Debian
set -e
apt-get update
apt-get install -y \
    meson \
    ninja-build \
    pkg-config \
    libgtk-4-dev \
    gcc \
    g++ \
    qemu-system-x86 \
    qemu-utils
echo "Dependencies installed."

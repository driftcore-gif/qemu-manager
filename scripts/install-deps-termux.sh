#!/bin/bash
# Install dependencies in Termux native (before proot-distro)
pkg install -y x11-repo
pkg install -y \
    meson \
    ninja \
    pkg-config \
    gtk4 \
    clang \
    qemu-system-x86-64 \
    qemu-system-aarch64 \
    qemu-utils
echo "Termux dependencies installed."
echo "Start X11: tx11start"
echo "Then run the app inside an X11 session."

#!/bin/bash
# Termux package build helper
PKGNAME=qemu-manager
VERSION=1.0.0

echo "Building $PKGNAME-$VERSION for Termux..."
meson setup build \
  --prefix=/data/data/com.termux/files/usr \
  --buildtype=release
cd build && ninja
echo "Binary: build/qemu-manager"
echo "Copy to Termux prefix: cp build/qemu-manager \$PREFIX/bin/"

#!/bin/bash
set -e
echo "[QEMU Manager] Setting up build..."
meson setup build --wipe 2>/dev/null || meson setup build
echo "[QEMU Manager] Building..."
cd build && ninja
echo ""
echo "Build complete! Run with: ./build/qemu-manager"

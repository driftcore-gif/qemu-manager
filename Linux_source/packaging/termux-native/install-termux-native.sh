#!/bin/bash
# ==============================================================================
# QEMU Manager — Native Termux Installer (manual install, no dpkg/.deb)
# ==============================================================================
# Downloads the prebuilt native QEMU Manager binary for your device's CPU
# architecture straight from GitHub Releases and installs it by hand into
# your Termux $PREFIX — no proot, no chroot, no dpkg/.deb package involved.
# ==============================================================================

set -e

REPO="driftcore-gif/qemu-manager"
TERMUX_PREFIX="${PREFIX:-/data/data/com.termux/files/usr}"

echo "=========================================================="
echo "  QEMU Manager — Native Termux Installer"
echo "=========================================================="

if [[ "$TERMUX_PREFIX" != *"com.termux"* ]] && [[ "${IGNORE_TERMUX_CHECK:-0}" != "1" ]]; then
    echo "ERROR: This installer must be run inside Termux on Android." >&2
    echo "(\$PREFIX does not contain 'com.termux'. Set IGNORE_TERMUX_CHECK=1 to force.)" >&2
    exit 1
fi

# ------------------------------------------------------------------------------
# 1. Ask which architecture to install
# ------------------------------------------------------------------------------
RAW_ARCH="$(uname -m)"
case "$RAW_ARCH" in
    aarch64|arm64) DETECTED="aarch64" ;;
    armv7l|armv8l|arm)  DETECTED="arm"     ;;
    x86_64)        DETECTED="x86_64" ;;
    i686|i386)     DETECTED="i686"   ;;
    *)             DETECTED="aarch64" ;;
esac

echo ""
echo "Detected device architecture: $RAW_ARCH  (=> $DETECTED)"
echo ""
echo "Available native builds:"
echo "  1) aarch64  — 64-bit ARM (nearly all modern Android phones)"
echo "  2) arm      — 32-bit ARM"
echo "  3) x86_64   — 64-bit Intel/AMD"
echo "  4) i686     — 32-bit Intel/AMD"
echo ""
read -r -p "Select architecture to install [1-4] (default: matches $DETECTED): " CHOICE

case "$CHOICE" in
    1) ARCH="aarch64" ;;
    2) ARCH="arm" ;;
    3) ARCH="x86_64" ;;
    4) ARCH="i686" ;;
    "") ARCH="$DETECTED" ;;
    *) echo "Invalid choice, using detected arch: $DETECTED"; ARCH="$DETECTED" ;;
esac

echo "[+] Installing native build for: $ARCH"

TARBALL="qemu-manager-termux-native-${ARCH}.tar.gz"
URL="https://github.com/${REPO}/releases/latest/download/${TARBALL}"

# ------------------------------------------------------------------------------
# 2. Download the tarball (or use a local copy sitting next to this script)
# ------------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

if [ -f "$SCRIPT_DIR/$TARBALL" ]; then
    echo "[+] Found local $TARBALL next to installer, using it."
    cp "$SCRIPT_DIR/$TARBALL" "$WORKDIR/$TARBALL"
else
    echo "[+] Downloading $TARBALL from GitHub Releases..."
    if command -v curl >/dev/null 2>&1; then
        curl -fL "$URL" -o "$WORKDIR/$TARBALL" || {
            echo "ERROR: could not download $URL" >&2
            echo "This architecture may not have a native build yet — try aarch64, or use the PRoot Debian method instead." >&2
            exit 1
        }
    else
        wget -O "$WORKDIR/$TARBALL" "$URL" || {
            echo "ERROR: could not download $URL" >&2
            exit 1
        }
    fi
fi

# ------------------------------------------------------------------------------
# 3. Runtime dependencies (GTK4 + libc++), only if missing
# ------------------------------------------------------------------------------
echo "[+] Checking runtime dependencies (gtk4, libc++)..."
if ! pkg list-installed 2>/dev/null | grep -q "^gtk4/"; then
    echo "[+] Installing x11-repo + gtk4 (one-time)..."
    pkg install x11-repo -y
    pkg install gtk4 -y
fi
if ! pkg list-installed 2>/dev/null | grep -q "^libc++/"; then
    pkg install libc++ -y
fi

# ------------------------------------------------------------------------------
# 4. Manually extract and install — no dpkg, no .deb
# ------------------------------------------------------------------------------
echo "[+] Extracting into $WORKDIR..."
tar xzf "$WORKDIR/$TARBALL" -C "$WORKDIR"

mkdir -p "$TERMUX_PREFIX/bin"
mkdir -p "$TERMUX_PREFIX/share/icons/hicolor/256x256/apps"
mkdir -p "$TERMUX_PREFIX/share/applications"

echo "[+] Installing binary to $TERMUX_PREFIX/bin/qemu-manager"
cp "$WORKDIR/bin/qemu-manager" "$TERMUX_PREFIX/bin/qemu-manager"
chmod 755 "$TERMUX_PREFIX/bin/qemu-manager"

if [ -f "$WORKDIR/share/icons/hicolor/256x256/apps/qemu-manager.png" ]; then
    cp "$WORKDIR/share/icons/hicolor/256x256/apps/qemu-manager.png" \
       "$TERMUX_PREFIX/share/icons/hicolor/256x256/apps/qemu-manager.png"
fi

cat > "$TERMUX_PREFIX/share/applications/qemu-manager.desktop" << 'EOF'
[Desktop Entry]
Name=QEMU Manager
Comment=Manage QEMU virtual machines (native Termux)
Exec=qemu-manager
Icon=qemu-manager
Terminal=false
Type=Application
Categories=System;Emulator;Virtualization;
EOF

# ------------------------------------------------------------------------------
# 5. Done
# ------------------------------------------------------------------------------
echo ""
echo "=========================================================="
echo "  Installed! Architecture: $ARCH"
echo "=========================================================="
echo ""
echo "  QEMU Manager needs a display. Install the 'Termux:X11' app"
echo "  (F-Droid / Play Store), then run:"
echo ""
echo "    termux-x11 :0 &"
echo "    export DISPLAY=:0"
echo "    qemu-manager"
echo ""
echo "  Don't forget QEMU itself:"
echo "    pkg install qemu-system-x86-64-headless   # or your target arch"
echo "=========================================================="

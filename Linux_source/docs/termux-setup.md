# Termux + PRoot Setup Guide

## Method 1: Native Termux (recommended for Termux-native GTK)

```bash
pkg install x11-repo
pkg install meson ninja pkg-config gtk4 clang qemu-system-x86-64 qemu-utils
git clone https://github.com/driftcore-gif/qemu-manager.git
cd qemu-manager
bash scripts/build.sh
tx11start &
DISPLAY=:1 ./build/qemu-manager
```

## Method 2: PRoot Debian inside Termux

```bash
pkg install proot-distro
proot-distro install debian
proot-distro login debian
```

Inside Debian PRoot:
```bash
bash scripts/install-deps-debian.sh
git clone https://github.com/driftcore-gif/qemu-manager.git
cd qemu-manager
bash scripts/build.sh
```

Run with X11 forwarding from Termux:
```bash
tx11start
DISPLAY=:1 proot-distro login debian -- /root/qemu-manager/build/qemu-manager
```

## Notes

- User networking (-netdev user) works without root in PRoot
- TAP/Bridge networking requires real root — not available in PRoot
- KVM acceleration is NOT available in PRoot/Termux — QEMU will use TCG emulation
- Add `-accel tcg,thread=multi` in Settings > Default Args for better performance
- For UEFI: install `ovmf` package inside PRoot

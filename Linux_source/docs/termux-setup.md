# Termux Setup Guide

## Method 0: Native Termux binary (recommended — no proot, no chroot)

As of v10.0, QEMU Manager ships a **genuinely native Termux binary** — cross-compiled
directly against Termux's own Bionic libc and Termux's prebuilt GTK4 package (from the
`x11-repo`), using the Android NDK. It runs straight inside Termux's own `$PREFIX`
filesystem. There is no PRoot, no Debian chroot, and no translation/emulation layer —
just a normal native ELF binary linked against `/data/data/com.termux/files/usr/lib`.

```bash
pkg install x11-repo -y
pkg install gtk4 libc++ -y

wget https://github.com/driftcore-gif/qemu-manager/releases/latest/download/qemu-manager-termux-native-10.0-1_aarch64.deb
dpkg -i qemu-manager-termux-native-10.0-1_aarch64.deb

pkg install qemu-system-x86-64-headless -y   # or whichever qemu-system-* you need

# GTK4 needs a display — install "Termux:X11" from F-Droid, then:
termux-x11 :0 &
export DISPLAY=:0
qemu-manager
```

Currently built for **aarch64** only (the architecture of nearly all modern Android
phones). If you need arm/i686/x86_64, see "Building it yourself" below — the same
cross-compilation recipe applies, just swap the NDK target triple and Termux's
`binary-<arch>` package index.

### How the native build works (for maintainers)

The native `.deb` is cross-compiled from a Debian/Linux build host — not on-device —
using this recipe:

1. Download the Android NDK (r27c or newer) for its clang/clang++ cross-compilers
   (`aarch64-linux-android24-clang++`, targeting API 24+, matching Termux's minimum
   supported Android version).
2. Download Termux's own prebuilt GTK4 package and its full dependency closure
   (glib, cairo, pango, gdk-pixbuf, harfbuzz, graphene, libx11, mesa, etc. — 83
   packages total) directly from `packages.termux.dev` (`termux-main` +
   `termux-x11` repos), and extract them into a local sysroot that mirrors
   `/data/data/com.termux/files/usr`.
3. Point Meson's `pkg-config` at that sysroot via `pkg_config_libdir` +
   `sys_root` in a cross file, so `dependency('gtk4')` resolves against Termux's
   real headers/libs instead of the host's.
4. Cross-compile with `meson setup --cross-file cross-termux-aarch64.ini` and
   `ninja`, then set `install_rpath = '/data/data/com.termux/files/usr/lib'` in
   `meson.build` so the shipped binary only looks for libraries in Termux's real
   library path (not the build host's temporary sysroot).
5. Package the resulting binary as a normal `dpkg-deb` archive with
   `Depends: gtk4, libc++` so `apt`/`dpkg` on-device pull in the runtime deps.

This is the same fundamental approach Termux's own package maintainers use
(`termux-packages` build system) — just done by hand against Termux's binary
repo instead of rebuilding every dependency from source.

## Method 1: Build from source natively in Termux

```bash
pkg install x11-repo
pkg install meson ninja pkg-config gtk4 clang qemu-system-x86-64 qemu-utils
git clone https://github.com/driftcore-gif/qemu-manager.git
cd qemu-manager/Linux_source
meson setup builddir
ninja -C builddir
termux-x11 :0 &
DISPLAY=:0 ./builddir/qemu-manager
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
termux-x11 :0 &
DISPLAY=:0 proot-distro login debian -- /root/qemu-manager/build/qemu-manager
```

## Notes

- User networking (-netdev user) works without root in both native Termux and PRoot.
- TAP/Bridge networking requires real root — not available in either environment.
- The native Termux binary (Method 0) has no proot overhead — QEMU itself runs at
  full native speed either way, but the manager UI and process supervision has less
  indirection.

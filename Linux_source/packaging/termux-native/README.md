# Native Termux cross-compilation toolkit

Reproduces the native (no-proot) Termux aarch64 build. See
`../../docs/termux-setup.md` for the full explanation.

## Quick steps

```bash
# 1. Get the Android NDK (r27c+) — provides aarch64-linux-android24-clang++
curl -LO https://dl.google.com/android/repository/android-ndk-r27c-linux.zip
unzip android-ndk-r27c-linux.zip

# 2. Download Termux's Packages index and resolve the gtk4 dependency closure
mkdir repo && cd repo
curl -LO https://packages.termux.dev/apt/termux-main/dists/stable/main/binary-aarch64/Packages.gz
curl -LO https://packages.termux.dev/apt/termux-x11/dists/x11/main/binary-aarch64/Packages.gz
gunzip -k *.gz
cd ..
python3 resolve-termux-deps.py   # writes resolved_pkgs.json with all 83 packages + URLs

# 3. Download every resolved .deb and extract into a local sysroot
#    (mirrors /data/data/com.termux/files/usr)
#    then dpkg-deb -x each .deb into ./sysroot

# 4. Update the absolute paths inside cross-termux-aarch64.ini to match
#    your NDK + sysroot locations, then:
cd ../../..   # back to Linux_source/
meson setup builddir-termux-aarch64 --cross-file packaging/termux-native/cross-termux-aarch64.ini \
  --prefix=/data/data/com.termux/files/usr
ninja -C builddir-termux-aarch64
DESTDIR=./stage ninja -C builddir-termux-aarch64 install

# 5. Package as a plain tarball (NOT a .deb — no dpkg involved)
tar czf qemu-manager-termux-native-aarch64.tar.gz \
  -C stage/data/data/com.termux/files/usr \
  bin/qemu-manager share/icons/hicolor/256x256/apps/qemu-manager.png
```

## Installing it on-device

We deliberately do **not** ship this as a `.deb`. Distribution is a plain
tarball plus `install-termux-native.sh`, which asks the user which
architecture they're on and copies the binary + icon into `$PREFIX` by hand
(`cp` + `chmod`) — no `dpkg`, no package database, no postinst scripts.

```bash
curl -LO https://github.com/driftcore-gif/qemu-manager/releases/latest/download/install-termux-native.sh
bash install-termux-native.sh
```


## Why this works

Termux publishes prebuilt GTK4 (and its ~80 transitive deps) for aarch64/arm/i686/x86_64
in its own apt repos. We don't need to cross-compile GTK4 ourselves — only our own app,
linked against Termux's already-built libraries and headers via a Meson cross file that
redirects pkg-config into the extracted sysroot (`pkg_config_libdir` + `sys_root`).
`install_rpath` in `meson.build` makes sure the shipped binary points at Termux's real
runtime library path, not the build machine's temporary sysroot.

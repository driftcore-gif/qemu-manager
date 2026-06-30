<div align="center">

<img src="Linux_source/qemu-manager-icon.svg" alt="QEMU Manager" width="96"/>

# QEMU Manager

**A lightweight, libvirt-free QEMU virtual machine manager for Linux desktops and Android.**  
Designed for PRoot, Termux, and standard Debian/Ubuntu environments — no root, no systemd, no daemon required.

[![Release](https://img.shields.io/github/v/release/driftcore-gif/qemu-manager?color=4FC3F7&label=Latest)](https://github.com/driftcore-gif/qemu-manager/releases/latest)
[![License](https://img.shields.io/github/license/driftcore-gif/qemu-manager?color=4FC3F7)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Android-lightgrey)](https://github.com/driftcore-gif/qemu-manager/releases)

</div>

---

## Overview

QEMU Manager provides a clean GUI for managing QEMU virtual machines without requiring libvirt, systemd, root access, or any background daemon. It works equally well on a full Linux desktop and inside Android via Termux or a PRoot Debian environment.

| Feature | Linux Desktop | Android |
|---|---|---|
| Create / edit / delete VMs | ✅ | ✅ |
| Per-VM hardware config | ✅ | ✅ |
| Binary auto-detection | ✅ | ✅ |
| Copy QEMU command | ✅ | ✅ |
| QEMU Monitor | ✅ | ✅ |
| GTK4 native UI | ✅ | — |
| Material Design UI | — | ✅ |

---

## Download

Pre-built packages for every supported platform are attached to the [latest release](https://github.com/driftcore-gif/qemu-manager/releases/latest):

| Package | Architecture | Use case |
|---|---|---|
| `qemu-manager_*_amd64.deb` | x86_64 | Standard PC / server |
| `qemu-manager_*_arm64.deb` | aarch64 | ARM64 SBC, Raspberry Pi 4/5 |
| `qemu-manager_*_armhf.deb` | ARMv7 (hard-float) | 32-bit ARM SBC |
| `qemu-manager_*_i386.deb` | x86 32-bit | Legacy / chroot |
| `qemu-manager_*_android.apk` | Universal ARM/x86 | Android + Termux/PRoot |

---

## Linux Installation

### From .deb (Debian / Ubuntu / Raspberry Pi OS)

```bash
# Pick your architecture — example: amd64
wget https://github.com/driftcore-gif/qemu-manager/releases/latest/download/qemu-manager_7.0-1_amd64.deb
sudo dpkg -i qemu-manager_7.0-1_amd64.deb

# Install QEMU if not already present
sudo apt install qemu-system-x86
```

### Build from source

```bash
# Prerequisites
sudo apt install meson ninja-build pkg-config libgtk-4-dev

# Clone and build
git clone https://github.com/driftcore-gif/qemu-manager.git
cd qemu-manager/Linux_source
meson setup builddir
cd builddir && ninja
sudo ninja install
```

---

## Android Installation

### Option 1 — Native install (sideload)

1. Enable **Unknown Sources** in Android Settings → Security
2. Download `qemu-manager_*_android.apk` from [Releases](https://github.com/driftcore-gif/qemu-manager/releases/latest)
3. Tap the APK to install

### Option 2 — Termux + QEMU

```bash
# Install QEMU in Termux
pkg install qemu-system-x86-64-headless   # or qemu-system-aarch64, etc.

# Use QEMU Manager to create VMs and copy the generated command
# Run the copied command directly in Termux
```

### Option 3 — PRoot Debian in Termux

```bash
pkg install proot-distro
proot-distro install debian
proot-distro login debian
apt install qemu-system-x86   # inside PRoot
# Then run QEMU Manager; binary paths are auto-detected
```

---

## Repository Structure

```
qemu-manager/
├── Android_source/       ← Native Android app (Java / Gradle)
│   ├── app/
│   │   └── src/main/
│   │       ├── java/com/qemumanager/
│   │       │   ├── adapter/     VMAdapter.java
│   │       │   ├── model/       VMConfig.java
│   │       │   ├── ui/          MainActivity, VMWizard, VMDetail, VMEdit, Monitor, Settings
│   │       │   └── util/        CommandBuilder, VMConfigIO, VMLauncher
│   │       └── res/             Layouts, drawables, values
│   └── build.gradle
│
└── Linux_source/         ← GTK4 C++ desktop application
    ├── src/              Application source files (.cpp)
    ├── include/          Header files (.h)
    ├── meson.build       Build definition
    ├── cross/            Cross-compilation toolchain files
    ├── packaging/        .deb packaging templates
    ├── scripts/          Build & release helper scripts
    └── docs/             Additional documentation
```

---

## VM Configuration

VMs are stored as JSON files in:
- **Linux:** `~/.local/share/qemu-manager/VMs/<vm-name>/vm.json`
- **Android:** `/sdcard/Android/data/com.qemumanager/files/VMs/<vm-name>/vm.json`

Each VM config captures:

| Category | Options |
|---|---|
| **General** | Name, description, architecture (x86_64, aarch64, arm, riscv64, …), mode (System / User) |
| **Machine** | pc, q35, virt, microvm, custom |
| **CPU** | Model, sockets, cores, threads |
| **Memory** | RAM (64 MB – 32 GB), VirtIO balloon |
| **Storage** | Disk image path, size, format (qcow2, raw, vmdk, vdi), ISO |
| **Boot** | Order, UEFI/OVMF, boot menu |
| **Display** | SDL, GTK, SPICE, VNC, headless |
| **GPU** | VGA, VirtIO-GPU, QXL, Cirrus, VMWARE |
| **Audio** | Intel HDA, AC97, SB16, none |
| **Network** | User (NAT), TAP, Bridge, Socket |
| **Hardware** | QEMU binary (auto-detected or custom name), accelerator (TCG / KVM / KVM+LBT), extra arguments |

---

## Accelerators

| Accelerator | Requirement | Description |
|---|---|---|
| **TCG** | None | Software emulation — works everywhere, slower |
| **KVM** | Linux kernel + `/dev/kvm` | Hardware virtualisation — near-native speed |
| **KVM + LBT** | LoongArch CPU with LBT + KVM kernel | LoongArch Binary Translation: runs x86/MIPS/ARM binaries natively |

---

## QEMU Binary Detection

QEMU Manager automatically scans these paths at runtime:

| Path | Environment |
|---|---|
| `/usr/bin/` | Standard Linux, PRoot Debian |
| `/data/data/com.termux/files/usr/bin/` | Termux native |
| `/system/bin/` | Android system |

Custom binary names (e.g. `qemu-system-loongarch64`) can be entered manually — they are resolved against the same paths at launch time.

---

## Building the Android APK

```bash
cd Android_source

# Debug build
./gradlew assembleDebug
# Output: app/build/outputs/apk/debug/app-debug.apk

# Release build (requires keystore)
./gradlew assembleRelease
```

Requirements: Android SDK 34, Java 17, Gradle 8.6

---

## Cross-Compiling Linux Binaries

```bash
cd Linux_source

# aarch64 (ARM64)
meson setup builddir-aarch64 --cross-file cross/aarch64-linux-gnu.ini
cd builddir-aarch64 && ninja

# armhf
meson setup builddir-armhf --cross-file cross/arm-linux-gnueabihf.ini
cd builddir-armhf && ninja

# i386
meson setup builddir-i386 --cross-file cross/i686-linux-gnu.ini
cd builddir-i386 && ninja
```

---

## License

[GNU General Public License v3.0](LICENSE)

---

## Contributing

See [CONTRIBUTING.md](Linux_source/CONTRIBUTING.md) for guidelines.  
Bug reports and pull requests are welcome.

---

<div align="center">
Made for the Termux / PRoot community · No root required · No daemons · Just QEMU
</div>

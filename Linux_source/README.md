# QEMU Manager

A native GTK4 desktop application that manages QEMU virtual machines by generating and launching QEMU commands directly.

**No root required. No libvirt. No systemd. Works in Termux + PRoot.**

![License](https://img.shields.io/badge/license-GPL--2.0-blue)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Termux-green)
![GTK](https://img.shields.io/badge/GTK-4.x-orange)
![Language](https://img.shields.io/badge/language-C%2B%2B17-red)

---

## Features

- Full VM wizard — CPU, RAM, disk, display, audio, network, USB, boot
- 10 architecture targets: x86_64, i386, aarch64, arm, riscv64, riscv32, mips, mipsel, ppc64, sparc
- Live QEMU command builder and preview
- Snapshot management via qemu-img
- ISO library browser
- VM templates (Ubuntu, Debian, Fedora, Arch, Alpine, Windows, Android, ARM, RISC-V)
- Download manager with ISO and firmware links
- Logs viewer and serial console
- Dark / light / system theme
- Generates `start.sh` for each VM
- Works in Termux + PRoot without root

---

## Screenshots

> Add screenshots here after first build.

---

## Requirements

### Linux (Debian / Ubuntu)
```bash
sudo apt install meson ninja-build pkg-config libgtk-4-dev qemu-system-x86
```

### Arch Linux
```bash
sudo pacman -S meson ninja pkg-config gtk4 qemu-full
```

### Fedora
```bash
sudo dnf install meson ninja-build pkg-config gtk4-devel qemu
```

### Termux + PRoot (Debian inside Termux)
```bash
pkg install x11-repo
pkg install meson ninja pkg-config gtk4 qemu-system-x86-64
```

---

## Build

```bash
git clone https://github.com/driftcore-gif/qemu-manager.git
cd qemu-manager
meson setup build
cd build
ninja
./qemu-manager
```

Or use the helper script:
```bash
bash scripts/build.sh
```

---

## Install (optional)

```bash
cd build
sudo ninja install
```

---

## VM Storage Layout

Each VM is stored as a directory under `~/VMs/`:

```
~/VMs/
 └── Ubuntu/
      ├── vm.json       ← configuration
      ├── disk.qcow2    ← disk image
      ├── start.sh      ← generated launch script
      ├── nvram.fd      ← UEFI NVRAM (if UEFI enabled)
      ├── snapshots/
      └── logs/
           └── qemu.log
```

---

## Example Generated Command

```bash
qemu-system-x86_64 \
  -machine q35 \
  -cpu max \
  -smp 4,sockets=1,cores=2,threads=2 \
  -m 4096 \
  -drive file=disk.qcow2,if=virtio,format=qcow2 \
  -cdrom ubuntu.iso \
  -boot order=cd,menu=on \
  -display gtk \
  -device virtio-gpu-pci \
  -device intel-hda \
  -device hda-duplex \
  -usb \
  -device usb-tablet \
  -netdev user,id=net0 \
  -device virtio-net-pci,netdev=net0 \
  -serial stdio \
  -monitor unix:/root/VMs/Ubuntu/monitor.sock,server,nowait \
  -pidfile /root/VMs/Ubuntu/vm.pid
```

---

## Architecture Support

| Architecture | QEMU Binary              |
|-------------|--------------------------|
| x86_64      | qemu-system-x86_64       |
| i386        | qemu-system-i386         |
| aarch64     | qemu-system-aarch64      |
| arm         | qemu-system-arm          |
| riscv64     | qemu-system-riscv64      |
| riscv32     | qemu-system-riscv32      |
| mips        | qemu-system-mips         |
| mipsel      | qemu-system-mipsel       |
| ppc64       | qemu-system-ppc64        |
| sparc       | qemu-system-sparc        |

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

1. Fork the repo
2. Create a branch: `git checkout -b feature/your-feature`
3. Commit: `git commit -m "Add your feature"`
4. Push: `git push origin feature/your-feature`
5. Open a Pull Request

---

## Roadmap

- [ ] Plugin system
- [ ] VirtIO-FS shared folders
- [ ] Drag-and-drop ISO support
- [ ] VM export/import bundles
- [ ] Notifications
- [ ] Keyboard shortcuts
- [ ] Command history browser
- [ ] Auto VM state save
- [ ] Flatpak packaging
- [ ] AppImage packaging

---

## License

GPL-2.0 — see [LICENSE](LICENSE).

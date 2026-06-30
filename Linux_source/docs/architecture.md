# Architecture

## Overview

QEMU Manager is a thin GTK4 frontend that:
1. Stores VM configs as JSON files on disk
2. Builds QEMU command-line arguments from those configs
3. Launches QEMU as a child process
4. Monitors the process via PID file and UNIX socket

No daemon. No libvirt. No root. No systemd.

## Source Layout

```
src/
  main.cpp           Entry point
  app.cpp            GtkApplication lifecycle
  mainwindow.cpp     Main window — sidebar, toolbar, page stack
  vm.cpp             VMConfig struct, JSON save/load (no external deps)
  vmwizard.cpp       9-tab VM creation wizard
  commandbuilder.cpp Builds QEMU CLI from VMConfig
  storagemanager.cpp qemu-img wrappers (create, resize, convert, clone)
  snapshotmanager.cpp qemu-img snapshot wrappers
  dashboard.cpp      Live system stats cards
  vmlistview.cpp     VM list row builder
  logviewer.cpp      Log file text view
  serialconsole.cpp  Serial console UI
  networkconfig.cpp  Network config widget
  displayconfig.cpp  Display/GPU config widget
  templates.cpp      Built-in VM templates
  downloadmanager.cpp Download links manager
  settings.cpp       JSON settings load/save

include/
  vm.h               VMConfig, enums, structs
  commandbuilder.h
  storagemanager.h
  snapshotmanager.h
  settings.h
  mainwindow.h
  vmwizard.h
  app.h
  templates.h
```

## VM Config (vm.json)

```json
{
  "name": "Ubuntu",
  "arch": "x86_64",
  "machine": "q35",
  "cpu_model": "max",
  "sockets": 1,
  "cores": 2,
  "threads": 2,
  "ram_mb": 4096,
  "disk_path": "/root/VMs/Ubuntu/disk.qcow2",
  "disk_format": "qcow2",
  "disk_size_gb": 25,
  "iso_path": "/root/ISOs/ubuntu.iso",
  "uefi": true,
  "boot_order": "cd",
  "display": 0,
  "gpu": 1,
  "audio": 0,
  "net_mode": 0,
  "extra_args": ""
}
```

## Command Builder

CommandBuilder::buildCommand() maps VMConfig fields to QEMU flags:

- arch → qemu-system-{arch} binary
- machine → -machine
- cpu_model + sockets/cores/threads → -cpu, -smp
- ram_mb → -m
- disk_path + disk_format → -drive file=...,if=virtio,format=...
- iso_path → -cdrom
- uefi → -bios /usr/share/ovmf/OVMF.fd
- display → -display gtk/sdl/vnc/...
- gpu → -device virtio-gpu-pci / VGA / qxl-vga / ...
- audio → -device intel-hda + hda-duplex / AC97 / sb16
- net_mode → -netdev user,id=net0 + -device virtio-net-pci,netdev=net0
- shared_folders → -fsdev + -device virtio-9p-pci
- extra_args → appended raw

## GTK4 Compatibility Notes

- Uses gtk_alert_dialog_new() — NOT deprecated gtk_message_dialog_new()
- Uses gtk_file_dialog_open() async — NOT deprecated gtk_file_chooser_dialog_new()
- Uses gtk_file_dialog_select_folder() async — NOT deprecated folder chooser
- Minimum GTK4 version: 4.10

# Changelog

All notable changes to QEMU Manager are documented here.

Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)

## [Unreleased]

### Added
- Initial project structure
- GTK4 main window with sidebar navigation
- VM wizard (General, CPU, Memory, Storage, Boot, Display, Network, Advanced, Preview)
- QEMU command builder for 10 architectures
- Snapshot management via qemu-img
- ISO library browser
- VM templates: Ubuntu, Debian, Fedora, Arch, Alpine, Tiny Core, Windows, Android x86, ARM64, RISC-V
- Download manager with ISO/firmware links
- Settings page (QEMU binary path, VM folder, ISO folder)
- Log viewer
- Serial console UI
- Dashboard (CPU, RAM, running/stopped VMs, QEMU version)
- GTK4 async file dialogs (no deprecated APIs)
- start.sh generation per VM
- Works in Termux + PRoot without root

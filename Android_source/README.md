# QEMU Manager — Android

Native Android app to manage QEMU virtual machines on Android + Termux/PRoot.
Same feature set as the desktop GTK4 version.

## Requirements
- Android 8.0+ (API 26)
- Termux or PRoot with QEMU installed in /usr/bin/
- For KVM: Android kernel with KVM support + /dev/kvm access

## Features
- Create VMs with 35 architectures
- Per-VM binary selection: detected /usr/bin/qemu-system-* dropdown + Custom override
- Per-VM accelerator: TCG / KVM / KVM+LBT (LoongArch silicon translation)
- Extra arguments appended last
- QEMU Monitor: send commands to running VMs
- vm.json config (same format as desktop version — cross-compatible)
- start.sh optional save per VM

## Build
```
./gradlew assembleDebug
# APK at: app/build/outputs/apk/debug/app-debug.apk
```

## Install
```
adb install app/build/outputs/apk/debug/app-debug.apk
```

## VM Storage
VMs are stored at:
```
/sdcard/Android/data/com.qemumanager/files/VMs/<vm-name>/vm.json
```
Same vm.json format as desktop — configs are portable between desktop and Android.

## Architecture
- `model/VMConfig.java`    — VM data model (matches desktop vm.h exactly)
- `util/CommandBuilder.java` — builds QEMU argv (same logic as desktop)
- `util/VMConfigIO.java`   — read/write vm.json
- `util/VMLauncher.java`   — fork QEMU process in-memory (no /dev/shm)
- `ui/MainActivity.java`   — VM list
- `ui/VMWizardActivity.java` — VM creation wizard (all settings in one scroll)
- `ui/VMDetailActivity.java` — VM detail + start/stop/delete/edit/monitor
- `ui/VMEditActivity.java`   — edit hardware/accel/extra-args
- `ui/QEMUMonitorActivity.java` — QEMU monitor terminal

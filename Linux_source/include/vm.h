#pragma once
#include <string>
#include <vector>

enum class VMArch {
    x86_64, i386,
    aarch64, arm, armeb,
    riscv64, riscv32,
    mips, mipsel, mips64, mips64el, mipsn32, mipsn32el,
    ppc, ppc64, ppc64le,
    sparc, sparc64, sparc32plus,
    alpha, hppa, m68k,
    microblaze, microblazeel,
    or1k, loongarch64, s390x,
    sh4, sh4eb,
    xtensa, xtensaeb,
    tricore, rx, avr, hexagon
};

enum class VMMode      { System, UserMode };
enum class VMStatus    { Stopped, Running, Paused, Suspended };
enum class MachineType { pc, q35, virt, microvm, custom };
enum class DisplayType { GTK, SDL, SPICE, VNC, EGL, Headless };
enum class NetworkMode { User, TAP, Bridge, Socket };
enum class DiskFormat  { qcow2, raw, vmdk, vdi };
enum class GPUType     { VGA, VirtIO, QXL, Cirrus, VMwareSVGA, None };
enum class AudioType   { IntelHDA, AC97, SB16, None };

// Per-VM accelerator
// TCG   : -accel tcg (software, always works)
// KVM   : -accel kvm (hardware /dev/kvm)
// KVM_LBT: -accel kvm -cpu max,lbt=on  (LoongArch silicon BT)
enum class Accelerator { TCG, KVM, KVM_LBT };

// Per-VM QEMU binary mode
// Auto   : derive from arch  e.g. qemu-system-aarch64
// Custom : user-supplied name (binary name only, resolved in /usr/bin/)
enum class BinaryMode  { Auto, Custom };

struct PortForward  { std::string protocol; int host_port; int guest_port; };
struct USBDevice    { std::string type; };
struct SharedFolder { std::string host_path; std::string guest_path; bool read_only = false; };

struct VMConfig {
    // Identity
    std::string name, description, icon;
    std::vector<std::string> tags;

    // Architecture
    VMArch      arch    = VMArch::x86_64;
    VMMode      mode    = VMMode::System;
    MachineType machine = MachineType::q35;
    std::string machine_custom;

    // CPU
    std::string cpu_model = "max";
    int sockets = 1, cores = 2, threads = 2;

    // Memory
    int  ram_mb     = 2048;
    bool ballooning = false;

    // Storage
    std::string disk_path, iso_path;
    DiskFormat  disk_format  = DiskFormat::qcow2;
    int         disk_size_gb = 20;

    // Boot
    bool        uefi = false, secure_boot = false, boot_menu = false;
    std::string boot_order = "cd";

    // Display
    DisplayType display    = DisplayType::SDL;   // SDL default
    int         res_width  = 1280, res_height = 720;
    bool        fullscreen = false;
    GPUType     gpu        = GPUType::VirtIO;
    AudioType   audio      = AudioType::IntelHDA;

    // USB / Network / Shares
    std::vector<USBDevice>    usb_devices;
    NetworkMode               net_mode = NetworkMode::User;
    std::vector<PortForward>  port_forwards;
    std::vector<SharedFolder> shared_folders;

    // ---- Per-VM binary selection ----
    BinaryMode  binary_mode   = BinaryMode::Auto;
    std::string custom_binary; // name only e.g. "qemu-system-mycpu"
                               // always resolved as /usr/bin/<custom_binary>

    // ---- Per-VM accelerator ----
    Accelerator accel = Accelerator::TCG;

    // ---- Extra QEMU arguments (appended last) ----
    std::string extra_args;

    // Save start.sh to VM folder?
    bool save_script_to_vm_folder = false;

    // Runtime
    VMStatus    status = VMStatus::Stopped;
    std::string last_started, vm_dir;
};

struct Snapshot { std::string name, date, notes; };

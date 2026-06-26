#pragma once
#include <string>
#include <vector>
#include <map>

enum class VMStatus { Stopped, Running, Paused, Suspended };
enum class VMArch { x86_64, i386, aarch64, arm, riscv64, riscv32, mips, mipsel, ppc64, sparc };
enum class MachineType { pc, q35, virt, microvm };
enum class DisplayType { GTK, SDL, SPICE, VNC, EGL, Headless };
enum class NetworkMode { User, TAP, Bridge, Socket };
enum class DiskFormat { qcow2, raw, vmdk, vdi };
enum class GPUType { VGA, VirtIO, QXL, Cirrus, VMwareSVGA, None };
enum class AudioType { IntelHDA, AC97, SB16, None };

struct PortForward {
    std::string protocol;
    int host_port;
    int guest_port;
};

struct USBDevice {
    std::string type; // tablet, mouse, keyboard, storage, webcam
};

struct SharedFolder {
    std::string host_path;
    std::string guest_path;
    bool read_only;
};

struct VMConfig {
    // General
    std::string name;
    std::string description;
    std::string icon;
    std::vector<std::string> tags;

    // Architecture
    VMArch arch = VMArch::x86_64;
    MachineType machine = MachineType::q35;

    // CPU
    std::string cpu_model = "max";
    int sockets = 1;
    int cores = 2;
    int threads = 2;

    // Memory
    int ram_mb = 2048;
    bool ballooning = false;
    bool huge_pages = false;

    // Storage
    std::string disk_path;
    DiskFormat disk_format = DiskFormat::qcow2;
    int disk_size_gb = 20;
    std::string iso_path;

    // Boot
    bool uefi = false;
    bool secure_boot = false;
    std::string boot_order = "cd";
    bool boot_menu = false;

    // Display
    DisplayType display = DisplayType::GTK;
    int res_width = 1280;
    int res_height = 720;
    bool fullscreen = false;
    GPUType gpu = GPUType::VirtIO;

    // Audio
    AudioType audio = AudioType::IntelHDA;

    // USB
    std::vector<USBDevice> usb_devices;

    // Network
    NetworkMode net_mode = NetworkMode::User;
    std::vector<PortForward> port_forwards;

    // Shared folders
    std::vector<SharedFolder> shared_folders;

    // Extra args
    std::string extra_args;

    // State
    VMStatus status = VMStatus::Stopped;
    std::string last_started;
    std::string vm_dir;
};

struct Snapshot {
    std::string name;
    std::string date;
    std::string notes;
};

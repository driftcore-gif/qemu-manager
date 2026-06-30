#pragma once
#include <string>
#include <vector>

// ── Architecture ──────────────────────────────────────────────────────
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
    sh4, sh4eb, xtensa, xtensaeb,
    tricore, rx, avr, hexagon
};

enum class VMMode   { System, UserMode };
enum class VMStatus { Stopped, Running, Paused, Suspended };

// ── Machine ───────────────────────────────────────────────────────────
enum class MachineType {
    pc, q35, virt, microvm,
    sbsa_ref,           // ARM SBSA reference platform
    virt_acpi,          // QEMU 11: virt with ACPI
    x86_64_microvm,     // optimised microvm for x86
    custom
};

// ── Display ───────────────────────────────────────────────────────────
enum class DisplayType { GTK, SDL, SPICE, VNC, EGL, DBus, Headless };

// ── GPU ───────────────────────────────────────────────────────────────
enum class GPUType {
    VGA,
    VirtIO_GPU,          // virtio-gpu-pci (2D)
    VirtIO_GPU_GL,       // virtio-gpu-gl-pci  ← VirGL / OpenGL
    VirtIO_GPU_Rutabaga, // virtio-gpu-rutabaga (Android GPU stack)
    QXL,
    Cirrus,
    VMwareSVGA,
    ramfb,               // QEMU 8+: simple framebuffer
    None
};

// ── TCG Translation Block cache ───────────────────────────────────────
enum class TbSize { MB64, MB128, MB256, MB512, MB1024 };

// ── Audio ─────────────────────────────────────────────────────────────
enum class AudioType { IntelHDA, AC97, SB16, VirtIO_Sound, None };
// VirtIO_Sound = QEMU 8+: virtio-sound-pci

// ── Network ───────────────────────────────────────────────────────────
enum class NetworkMode { User, TAP, Bridge, Socket, VDE, VirtIO_VHostNet };

// ── Storage ───────────────────────────────────────────────────────────
enum class DiskFormat    { qcow2, raw, vmdk, vdi };
enum class DiskInterface { virtio, scsi, nvme, ide, sata };

// ── Accelerator ───────────────────────────────────────────────────────
enum class Accelerator { TCG, KVM, KVM_LBT, WHPX, HVF, NVMM, Xen };

// ── Firmware ──────────────────────────────────────────────────────────
enum class Firmware { SeaBIOS, OVMF, OVMF_SecureBoot, U_Boot, EDK2_ARM };

// ── USB ───────────────────────────────────────────────────────────────
enum class UsbVersion { USB3_XHCI, USB2_EHCI };

// ── IOMMU (QEMU 11) ───────────────────────────────────────────────────
enum class IommuType { None, Intel, SMMUv3, VirtIO_IOMMU };

// ── TPM (QEMU 8+) ─────────────────────────────────────────────────────
enum class TpmType { None, TIS, CRB };

// ── Per-VM binary / accel modes ───────────────────────────────────────
enum class BinaryMode { Auto, Custom };

// ── Misc data structures ──────────────────────────────────────────────
struct PortForward  { std::string protocol; int host_port; int guest_port; };
struct USBDevice    { std::string type; };
struct SharedFolder { std::string host_path; std::string guest_path; bool read_only = false; };

// ══════════════════════════════════════════════════════════════════════
struct VMConfig {
    // Identity
    std::string name, description;

    // Architecture
    VMArch      arch         = VMArch::x86_64;
    VMMode      mode         = VMMode::System;
    MachineType machine      = MachineType::q35;
    std::string machine_custom;

    // CPU
    std::string cpu_model   = "max";
    std::string cpu_flags;          // extra -cpu flags e.g. "+avx2,migratable=off"
    bool        cpu_migratable = true;
    int         sockets = 1, cores = 2, threads = 2;

    // Memory
    int  ram_mb      = 2048;
    bool ballooning  = false;
    bool memfd_backend = false;     // QEMU 8+: memory-backend-memfd
    int  mem_slots   = 0;           // QEMU 11: hotplug slots

    // Storage
    std::string  disk_path, iso_path, disk2_path;
    DiskFormat   disk_format    = DiskFormat::qcow2;
    DiskInterface disk_interface= DiskInterface::virtio;
    int          disk_size_gb   = 20;
    bool         disk_discard   = true;  // discard=unmap,detect-zeroes=unmap
    bool         use_scsi_ctrl  = false; // virtio-scsi-pci controller
    int          nvme_namespaces= 1;

    // Boot / Firmware
    Firmware    firmware    = Firmware::SeaBIOS;
    bool        boot_menu   = false;
    std::string boot_order  = "cd";
    bool        secure_boot = false;

    // Display
    DisplayType display     = DisplayType::SDL;
    GPUType     gpu         = GPUType::VirtIO_GPU;
    AudioType   audio       = AudioType::IntelHDA;
    std::string resolution  = "1280x720";
    bool        virgl_enabled = false;

    // USB
    UsbVersion  usb_version = UsbVersion::USB3_XHCI;
    bool        usb_tablet  = true;

    // Network
    NetworkMode net_mode    = NetworkMode::User;
    std::string net_dns;
    std::string smb_share;
    std::vector<PortForward>  port_forwards;

    // IOMMU (QEMU 11)
    IommuType   iommu       = IommuType::None;

    // TPM (QEMU 8+)
    TpmType     tpm         = TpmType::None;

    // Hardware extras
    bool        virtio_rng  = false;
    bool        numa_enabled= false;

    // Binary
    BinaryMode  binary_mode   = BinaryMode::Auto;
    std::string custom_binary; // binary name only, resolved in /usr/bin/

    // Accelerator
    Accelerator accel       = Accelerator::TCG;
    TbSize      tb_size     = TbSize::MB256;  // TCG TB cache
    bool        kvm_nested  = false;
    bool        tcg_mttcg   = true;

    // QEMU 11 flags
    bool        snapshot_mode = false;  // -snapshot
    bool        no_reboot     = false;  // -no-reboot

    // Extra args + script
    std::string extra_args;
    bool        save_script_to_vm_folder = false;

    // Runtime
    VMStatus    status      = VMStatus::Stopped;
    std::string last_started, vm_dir;

    // Legacy compat fields (kept for old JSON loading)
    bool        uefi        = false;   // maps to firmware=OVMF on load
};

struct Snapshot { std::string name, date, notes; };

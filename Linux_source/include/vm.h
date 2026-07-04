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
    nitro_enclave,      // QEMU 11: AWS Nitro Enclave machine type
    amd_versal2_virt,   // QEMU 10.2: AMD Versal2 virtual platform
    custom
};

// ── Display ───────────────────────────────────────────────────────────
enum class DisplayType { GTK, SDL, SPICE, VNC, EGL, DBus, Headless };

// ── GPU ───────────────────────────────────────────────────────────────
enum class GPUType {
    VGA,
    VirtIO_GPU,              // virtio-gpu-pci (2D)
    VirtIO_GPU_GL,           // virtio-gpu-gl-pci  (VirGL/OpenGL)
    VirtIO_GPU_Rutabaga,     // virtio-gpu-rutabaga (Android GPU stack)
    VirtIO_GPU_NativeCtx,    // QEMU 11: native context drivers
    QXL,
    Cirrus,
    VMwareSVGA,
    ramfb,                   // QEMU 8+: simple framebuffer
    None
};

// ── TCG Translation Block cache ───────────────────────────────────────
enum class TbSize { MB64, MB128, MB256, MB512, MB1024 };

// ── Audio ─────────────────────────────────────────────────────────────
enum class AudioType { IntelHDA, AC97, SB16, VirtIO_Sound, None };

// ── Network ───────────────────────────────────────────────────────────
enum class NetworkMode { User, TAP, Bridge, Socket, VDE, VirtIO_VHostNet };

// ── 9pfs / Plan9 virtfs (QEMU 10.2: FreeBSD host support) ─────────────
enum class VirtFSDriver { None, Local_9P, Proxy_9P };

// ── Live migration mode (QEMU 10.2: cpr-exec) ─────────────────────────
enum class MigrationMode { None, CprExec, SaveVM };

// ── Storage ───────────────────────────────────────────────────────────
enum class DiskFormat    { qcow2, raw, vmdk, vdi };
enum class DiskInterface { virtio, scsi, nvme, ide, sata };

// ── Accelerator ───────────────────────────────────────────────────────
// QEMU 11: Nitro (AWS Nitro Enclaves), MSHV (Microsoft Hyper-V)
enum class Accelerator { TCG, KVM, KVM_LBT, WHPX, HVF, NVMM, Xen, Nitro, MSHV };

// ── Firmware ──────────────────────────────────────────────────────────
enum class Firmware { SeaBIOS, OVMF, OVMF_SecureBoot, U_Boot, EDK2_ARM };

// ── USB ───────────────────────────────────────────────────────────────
enum class UsbVersion { USB3_XHCI, USB2_EHCI };

// ── IOMMU (QEMU 11) ───────────────────────────────────────────────────
enum class IommuType { None, Intel, SMMUv3, VirtIO_IOMMU };

// ── TPM ───────────────────────────────────────────────────────────────
enum class TpmType { None, TIS, CRB };

// ── Confidential VM (QEMU 11 KVM) ─────────────────────────────────────
// SEV-SNP reset + TDX confidential VMs (requires matching hardware+KVM)
enum class ConfidentialVM { None, SEV_SNP, TDX };

// ── Per-VM binary / accel modes ───────────────────────────────────────
enum class BinaryMode { Auto, Custom };

// ── Misc data structures ──────────────────────────────────────────────
struct PortForward  { std::string protocol; int host_port; int guest_port; };
struct USBDevice    { std::string type; };
struct SharedFolder { std::string host_path; std::string guest_path; bool read_only = false; };

// ══════════════════════════════════════════════════════════════════════
struct VMConfig {
    std::string name, description;

    // Architecture
    VMArch      arch         = VMArch::x86_64;
    VMMode      mode         = VMMode::System;
    MachineType machine      = MachineType::q35;
    std::string machine_custom;

    // CPU
    std::string cpu_model    = "max";
    std::string cpu_flags;           // extra -cpu flags
    bool        cpu_migratable = true;
    int         sockets = 1, cores = 2, threads = 2;
    // QEMU 11: CET virtualisation (Intel 11th gen+)
    bool        kvm_cet      = false;
    // QEMU 10: Diamond Rapids / Sierra Forest v2 CPU support
    std::string x86_cpu_gen;         // e.g. "DiamondRapids", "SierraForest-v2"

    // Memory
    int  ram_mb        = 2048;
    bool ballooning    = false;
    bool memfd_backend = false;
    int  mem_slots     = 0;

    // Storage
    std::string   disk_path, iso_path, disk2_path;
    DiskFormat    disk_format     = DiskFormat::qcow2;
    DiskInterface disk_interface  = DiskInterface::virtio;
    int           disk_size_gb    = 20;
    bool          disk_discard    = true;
    bool          use_scsi_ctrl   = false;
    int           nvme_namespaces = 1;

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
    // QEMU 11: per-output unique resolution for virtio-gpu-gl multi-head
    std::string gpu_extra_outputs;   // comma-sep resolutions e.g. "1920x1080,2560x1440"

    // USB
    UsbVersion  usb_version = UsbVersion::USB3_XHCI;
    bool        usb_tablet  = true;

    // Network
    NetworkMode net_mode    = NetworkMode::User;
    std::string net_dns;
    std::string smb_share;
    std::vector<PortForward> port_forwards;

    // IOMMU (QEMU 11)
    IommuType   iommu       = IommuType::None;

    // TPM
    TpmType     tpm         = TpmType::None;

    // Confidential VM (QEMU 11 KVM)
    ConfidentialVM conf_vm  = ConfidentialVM::None;

    // Hardware extras
    bool        virtio_rng   = false;
    bool        numa_enabled = false;
    // QEMU 10: virtio-scsi multiqueue (one I/O thread per queue)
    bool        scsi_multiqueue = false;
    // QEMU 11: RISC-V IOMMU sys device
    bool        riscv_iommu = false;

    // QEMU 10.2: 9pfs virtfs shared filesystem (FreeBSD + Linux host)
    VirtFSDriver virtfs_driver = VirtFSDriver::None;
    std::string  virtfs_path;            // host path to share
    std::string  virtfs_mount_tag = "host_share";

    // QEMU 10.2: CPR-exec live migration (reduced resource usage on update)
    MigrationMode migration_mode = MigrationMode::None;

    // QEMU 10.2: io_uring main loop (performance, Linux 5.1+)
    bool         io_uring_loop = false;

    // Binary
    BinaryMode  binary_mode   = BinaryMode::Auto;
    std::string custom_binary;

    // Accelerator
    Accelerator accel      = Accelerator::TCG;
    TbSize      tb_size    = TbSize::MB256;
    bool        kvm_nested = false;
    bool        tcg_mttcg  = true;

    // QEMU 11 flags
    bool        snapshot_mode = false;
    bool        no_reboot     = false;

    // Extra args + script
    std::string extra_args;
    bool        save_script_to_vm_folder = false;

    // Runtime
    VMStatus    status = VMStatus::Stopped;
    std::string last_started, vm_dir;

    // Legacy compat
    bool        uefi = false;
};

struct Snapshot { std::string name, date, notes; };

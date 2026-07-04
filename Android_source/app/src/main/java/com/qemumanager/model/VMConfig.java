package com.qemumanager.model;

import org.json.JSONException;
import org.json.JSONObject;

public class VMConfig {

    // ── Enums ─────────────────────────────────────────────────────────
    public enum VMArch {
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
    }

    public enum VMMode      { System, UserMode }
    public enum VMStatus    { Stopped, Running, Paused, Suspended }

    public enum MachineType {
        pc, q35, virt, microvm,
        sbsa_ref,            // ARM SBSA reference platform
        virt_acpi,           // QEMU 11: virt with ACPI
        x86_64_microvm,      // optimised microvm for x86
        nitro_enclave,       // QEMU 11: AWS Nitro Enclave
        custom
    }

    public enum DisplayType { SDL, GTK, SPICE, VNC, EGL, Headless }
    public enum NetworkMode { User, TAP, Bridge, Socket }
    public enum DiskFormat  { qcow2, raw, vmdk, vdi }

    public enum GPUType {
        VGA, VirtIO_GPU, VirtIO_GPU_GL, VirtIO_GPU_Rutabaga,
        VirtIO_GPU_NativeCtx,  // QEMU 11: native context driver
        QXL, Cirrus, VMwareSVGA, ramfb, None
    }

    public enum AudioType   { IntelHDA, AC97, SB16, VirtIO_Sound, None }

    public enum Accelerator {
        TCG, KVM, KVM_LBT, WHPX, HVF, NVMM, Xen,
        Nitro,  // QEMU 11: AWS Nitro Enclave
        MSHV    // QEMU 11: Microsoft Hyper-V
    }

    public enum BinaryMode     { Auto, Custom }
    public enum IommuType      { None, Intel, SMMUv3, VirtIO_IOMMU }
    public enum TpmType        { None, TIS, CRB }
    public enum ConfidentialVM { None, SEV_SNP, TDX }

    // ── Fields ────────────────────────────────────────────────────────
    public String name        = "";
    public String description = "";

    // Architecture
    public VMArch      arch          = VMArch.x86_64;
    public VMMode      mode          = VMMode.System;
    public MachineType machine       = MachineType.q35;
    public String      machineCustom = "";

    // CPU
    public String  cpuModel   = "max";
    public String  cpuFlags   = "";
    public int     sockets    = 1;
    public int     cores      = 2;
    public int     threads    = 2;
    // QEMU 11: CET virtualisation
    public boolean kvmCet     = false;
    // QEMU 10: named CPU generation (DiamondRapids etc.)
    public String  x86CpuGen  = "";

    // Memory
    public int     ramMb      = 2048;
    public boolean ballooning = false;

    // Storage
    public String     diskPath   = "";
    public String     isoPath    = "";
    public DiskFormat diskFormat = DiskFormat.qcow2;
    public int        diskSizeGb = 20;
    public boolean    diskDiscard    = true;
    public boolean    useScsiCtrl   = false;
    public boolean    scsiMultiqueue = false;  // QEMU 10

    // Boot
    public boolean uefi      = false;
    public boolean bootMenu  = false;
    public String  bootOrder = "cd";

    // Display
    public DisplayType display = DisplayType.SDL;
    public GPUType     gpu     = GPUType.VirtIO_GPU;
    public AudioType   audio   = AudioType.IntelHDA;
    public boolean     virglEnabled = false;
    // QEMU 11: per-output unique resolution for virtio-gpu-gl multi-head
    public String  gpuExtraOutputs = "";

    // Network
    public NetworkMode net = NetworkMode.User;

    // Binary — always Auto for fixed arch; custom binary removed from UI
    public BinaryMode binaryMode   = BinaryMode.Auto;
    public String     customBinary = "";

    // Accelerator
    public Accelerator accel     = Accelerator.TCG;
    public boolean     tcgMttcg  = true;
    public boolean     kvmNested = false;

    // IOMMU (QEMU 11)
    public IommuType iommu = IommuType.None;
    // RISC-V IOMMU (QEMU 11)
    public boolean   riscvIommu = false;

    // TPM
    public TpmType tpm = TpmType.None;

    // Confidential VM (QEMU 11 KVM)
    public ConfidentialVM confVm = ConfidentialVM.None;

    // Misc flags
    public boolean snapshotMode      = false;
    public boolean noReboot          = false;
    public String  extraArgs         = "";
    public boolean saveScriptToFolder = false;

    // Runtime
    public VMStatus status      = VMStatus.Stopped;
    public String   vmDir       = "";
    public String   lastStarted = "";

    // ── Serialisation ─────────────────────────────────────────────────
    public JSONObject toJson() throws JSONException {
        JSONObject o = new JSONObject();
        o.put("name",        name);
        o.put("description", description);
        o.put("arch",        arch.name());
        o.put("mode",        mode.name());
        o.put("machine",     machine.name());
        o.put("machine_custom", machineCustom);
        o.put("cpu_model",   cpuModel);
        o.put("cpu_flags",   cpuFlags);
        o.put("sockets",     sockets);
        o.put("cores",       cores);
        o.put("threads",     threads);
        o.put("kvm_cet",     kvmCet);
        o.put("x86_cpu_gen", x86CpuGen);
        o.put("ram_mb",      ramMb);
        o.put("ballooning",  ballooning);
        o.put("disk_path",   diskPath);
        o.put("iso_path",    isoPath);
        o.put("disk_format", diskFormat.name());
        o.put("disk_size_gb",diskSizeGb);
        o.put("disk_discard", diskDiscard);
        o.put("use_scsi_ctrl", useScsiCtrl);
        o.put("scsi_multiqueue", scsiMultiqueue);
        o.put("uefi",        uefi);
        o.put("boot_menu",   bootMenu);
        o.put("boot_order",  bootOrder);
        o.put("display",     display.name());
        o.put("gpu",         gpu.name());
        o.put("audio",       audio.name());
        o.put("virgl_enabled", virglEnabled);
        o.put("gpu_extra_outputs", gpuExtraOutputs);
        o.put("net_mode",    net.name());
        o.put("binary_mode", binaryMode.name());
        o.put("custom_binary", customBinary);
        o.put("accel",       accel.name());
        o.put("tcg_mttcg",   tcgMttcg);
        o.put("kvm_nested",  kvmNested);
        o.put("iommu",       iommu.name());
        o.put("riscv_iommu", riscvIommu);
        o.put("tpm",         tpm.name());
        o.put("conf_vm",     confVm.name());
        o.put("snapshot_mode", snapshotMode);
        o.put("no_reboot",   noReboot);
        o.put("extra_args",  extraArgs);
        o.put("save_script_to_vm_folder", saveScriptToFolder);
        o.put("status",      "Stopped");
        o.put("vm_dir",      vmDir);
        o.put("last_started", lastStarted);
        return o;
    }

    public static VMConfig fromJson(JSONObject o) throws JSONException {
        VMConfig c = new VMConfig();
        c.name          = o.optString("name","");
        c.description   = o.optString("description","");
        try { c.arch    = VMArch.valueOf(o.optString("arch","x86_64")); }      catch(Exception ignored){}
        try { c.mode    = VMMode.valueOf(o.optString("mode","System")); }       catch(Exception ignored){}
        try { c.machine = MachineType.valueOf(o.optString("machine","q35")); }  catch(Exception ignored){}
        c.machineCustom = o.optString("machine_custom","");
        c.cpuModel      = o.optString("cpu_model","max");
        c.cpuFlags      = o.optString("cpu_flags","");
        c.sockets       = o.optInt("sockets",1);
        c.cores         = o.optInt("cores",2);
        c.threads       = o.optInt("threads",2);
        c.kvmCet        = o.optBoolean("kvm_cet",false);
        c.x86CpuGen     = o.optString("x86_cpu_gen","");
        c.ramMb         = o.optInt("ram_mb",2048);
        c.ballooning    = o.optBoolean("ballooning",false);
        c.diskPath      = o.optString("disk_path","");
        c.isoPath       = o.optString("iso_path","");
        try { c.diskFormat = DiskFormat.valueOf(o.optString("disk_format","qcow2")); } catch(Exception ignored){}
        c.diskSizeGb    = o.optInt("disk_size_gb",20);
        c.diskDiscard   = o.optBoolean("disk_discard",true);
        c.useScsiCtrl   = o.optBoolean("use_scsi_ctrl",false);
        c.scsiMultiqueue= o.optBoolean("scsi_multiqueue",false);
        c.uefi          = o.optBoolean("uefi",false);
        c.bootMenu      = o.optBoolean("boot_menu",false);
        c.bootOrder     = o.optString("boot_order","cd");
        try { c.display = DisplayType.valueOf(o.optString("display","SDL")); }   catch(Exception ignored){}
        try { c.gpu     = GPUType.valueOf(o.optString("gpu","VirtIO_GPU")); }    catch(Exception ignored){}
        try { c.audio   = AudioType.valueOf(o.optString("audio","IntelHDA")); }  catch(Exception ignored){}
        c.virglEnabled  = o.optBoolean("virgl_enabled",false);
        c.gpuExtraOutputs = o.optString("gpu_extra_outputs","");
        try { c.net     = NetworkMode.valueOf(o.optString("net_mode","User")); } catch(Exception ignored){}
        try { c.binaryMode = BinaryMode.valueOf(o.optString("binary_mode","Auto")); } catch(Exception ignored){}
        c.customBinary  = o.optString("custom_binary","");
        try { c.accel   = Accelerator.valueOf(o.optString("accel","TCG")); }     catch(Exception ignored){}
        c.tcgMttcg      = o.optBoolean("tcg_mttcg",true);
        c.kvmNested     = o.optBoolean("kvm_nested",false);
        try { c.iommu   = IommuType.valueOf(o.optString("iommu","None")); }      catch(Exception ignored){}
        c.riscvIommu    = o.optBoolean("riscv_iommu",false);
        try { c.tpm     = TpmType.valueOf(o.optString("tpm","None")); }          catch(Exception ignored){}
        try { c.confVm  = ConfidentialVM.valueOf(o.optString("conf_vm","None")); } catch(Exception ignored){}
        c.snapshotMode  = o.optBoolean("snapshot_mode",false);
        c.noReboot      = o.optBoolean("no_reboot",false);
        c.extraArgs     = o.optString("extra_args","");
        c.saveScriptToFolder = o.optBoolean("save_script_to_vm_folder",false);
        try { c.status  = VMStatus.valueOf(o.optString("status","Stopped")); }   catch(Exception ignored){}
        c.vmDir         = o.optString("vm_dir","");
        c.lastStarted   = o.optString("last_started","");
        return c;
    }
}

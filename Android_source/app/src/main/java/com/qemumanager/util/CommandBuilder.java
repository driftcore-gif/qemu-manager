package com.qemumanager.util;

import com.qemumanager.model.VMConfig;
import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class CommandBuilder {

    private static final String[] BIN_PATHS = {
        "/usr/bin",
        "/data/data/com.termux/files/usr/bin",
        "/system/bin",
        "/vendor/bin"
    };

    private static final String[] SYSTEM_BINS = {
        "qemu-system-x86_64","qemu-system-i386",
        "qemu-system-aarch64","qemu-system-arm","qemu-system-armeb",
        "qemu-system-riscv64","qemu-system-riscv32",
        "qemu-system-mips","qemu-system-mipsel",
        "qemu-system-mips64","qemu-system-mips64el",
        "qemu-system-mips","qemu-system-mipsel",
        "qemu-system-ppc","qemu-system-ppc64","qemu-system-ppc64",
        "qemu-system-sparc","qemu-system-sparc64","qemu-system-sparc",
        "qemu-system-alpha","qemu-system-hppa","qemu-system-m68k",
        "qemu-system-microblaze","qemu-system-microblazeel",
        "qemu-system-or1k","qemu-system-loongarch64","qemu-system-s390x",
        "qemu-system-sh4","qemu-system-sh4eb",
        "qemu-system-xtensa","qemu-system-xtensaeb",
        "qemu-system-tricore","qemu-system-rx",
        "qemu-system-avr","qemu-system-hexagon"
    };

    private static final String[] USER_BINS = {
        "qemu-x86_64","qemu-i386",
        "qemu-aarch64","qemu-arm","qemu-armeb",
        "qemu-riscv64","qemu-riscv32",
        "qemu-mips","qemu-mipsel","qemu-mips64","qemu-mips64el",
        "qemu-mipsn32","qemu-mipsn32el",
        "qemu-ppc","qemu-ppc64","qemu-ppc64le",
        "qemu-sparc","qemu-sparc64","qemu-sparc32plus",
        "qemu-alpha","qemu-hppa","qemu-m68k",
        "qemu-microblaze","qemu-microblazeel",
        "qemu-or1k","qemu-loongarch64","qemu-s390x",
        "qemu-sh4","qemu-sh4eb","qemu-xtensa","qemu-xtensaeb",
        "qemu-tricore","qemu-rx","qemu-avr","qemu-hexagon"
    };

    public static String getBinDir() {
        for (String p : BIN_PATHS) {
            File f = new File(p);
            if (f.exists() && f.isDirectory() && f.canRead()) return p;
        }
        return "/usr/bin";
    }

    public static List<String> getBinDirs() {
        List<String> dirs = new ArrayList<>();
        for (String p : BIN_PATHS) {
            File f = new File(p);
            if (f.exists() && f.isDirectory() && f.canRead()) dirs.add(p);
        }
        if (dirs.isEmpty()) dirs.add("/usr/bin");
        return dirs;
    }

    public static String archToQemuBin(VMConfig.VMArch arch, VMConfig.VMMode mode) {
        int i = arch.ordinal();
        String[] t = mode == VMConfig.VMMode.System ? SYSTEM_BINS : USER_BINS;
        return (i >= 0 && i < t.length) ? t[i] : "qemu-system-x86_64";
    }

    public static String resolveBinary(VMConfig vm) {
        String name = (vm.binaryMode == VMConfig.BinaryMode.Custom && !vm.customBinary.isEmpty())
            ? vm.customBinary : archToQemuBin(vm.arch, vm.mode);
        for (String dir : getBinDirs()) {
            File f = new File(dir, name);
            if (f.exists()) return dir + "/" + name;
        }
        return getBinDir() + "/" + name;
    }

    public static List<String> listInstalledBinaries(VMConfig.VMMode mode) {
        List<String> out = new ArrayList<>();
        String prefix = (mode == VMConfig.VMMode.System) ? "qemu-system-" : "qemu-";
        try {
            for (String dir : getBinDirs()) {
                File d = new File(dir);
                if (!d.exists() || !d.canRead()) continue;
                File[] files = d.listFiles();
                if (files == null) continue;
                for (File f : files) {
                    String n = f.getName();
                    if (n.startsWith(prefix)) {
                        if (mode == VMConfig.VMMode.UserMode && n.startsWith("qemu-system-")) continue;
                        if (!out.contains(n)) out.add(n);
                    }
                }
            }
            java.util.Collections.sort(out);
        } catch (Exception ignored) {}
        return out;
    }

    public static String validateCustomBinary(String name) {
        if (name == null || name.isEmpty()) return "Binary name is empty.";
        if (name.contains(" "))  return "No spaces allowed.";
        if (name.contains("/"))  return "No slashes allowed.";
        if (!name.startsWith("qemu-")) return "Must start with 'qemu-'.";
        return "";
    }

    // ── Build QEMU argument list ──────────────────────────────────────
    public static List<String> buildArgs(VMConfig vm) {
        List<String> args = new ArrayList<>();

        if (vm.mode == VMConfig.VMMode.UserMode) {
            if (!vm.extraArgs.isEmpty())
                args.addAll(Arrays.asList(vm.extraArgs.trim().split("[ \t]+")));
            return args;
        }

        // Machine
        String[] machMap = {
            "pc","q35","virt","microvm",
            "sbsa-ref",                 // sbsa_ref
            "virt,acpi=on",             // virt_acpi
            "microvm,x-option-roms=off",// x86_64_microvm
            "nitro-enclave",            // nitro_enclave
            ""                          // custom
        };
        String mach = (vm.machine == VMConfig.MachineType.custom)
            ? vm.machineCustom
            : machMap[Math.min(vm.machine.ordinal(), machMap.length - 1)];
        if (mach == null || mach.isEmpty()) mach = "q35";
        args.add("-machine"); args.add(mach);

        // Accelerator
        String cpuStr = (!vm.x86CpuGen.isEmpty()) ? vm.x86CpuGen : vm.cpuModel;
        switch (vm.accel) {
            case TCG:     args.add("-accel"); args.add("tcg,thread=" + (vm.tcgMttcg ? "multi" : "single")); break;
            case KVM:     args.add("-accel"); args.add("kvm"); break;
            case KVM_LBT: args.add("-accel"); args.add("kvm"); cpuStr += ",lbt=on"; break;
            case WHPX:    args.add("-accel"); args.add("whpx"); break;
            case HVF:     args.add("-accel"); args.add("hvf"); break;
            case NVMM:    args.add("-accel"); args.add("nvmm"); break;
            case Xen:     args.add("-accel"); args.add("xen"); break;
            case Nitro:   args.add("-accel"); args.add("nitro"); break;
            case MSHV:    args.add("-accel"); args.add("mshv"); break;
        }
        if (vm.kvmCet && (vm.accel == VMConfig.Accelerator.KVM || vm.accel == VMConfig.Accelerator.KVM_LBT))
            cpuStr += ",cet=on";
        if (vm.kvmNested && vm.accel == VMConfig.Accelerator.KVM)
            cpuStr += ",vmx=on";
        if (!vm.cpuFlags.isEmpty()) cpuStr += "," + vm.cpuFlags;
        args.add("-cpu"); args.add(cpuStr);

        // SMP
        args.add("-smp");
        args.add(vm.sockets * vm.cores * vm.threads
            + ",sockets=" + vm.sockets
            + ",cores=" + vm.cores
            + ",threads=" + vm.threads);

        // RAM
        args.add("-m"); args.add(String.valueOf(vm.ramMb));
        if (vm.ballooning) { args.add("-device"); args.add("virtio-balloon-pci"); }

        // UEFI / firmware
        if (vm.uefi) { args.add("-bios"); args.add("/usr/share/ovmf/OVMF.fd"); }

        // Disk
        if (!vm.diskPath.isEmpty()) {
            String[] fmts = {"qcow2","raw","vmdk","vdi"};
            String fmt = fmts[Math.min(vm.diskFormat.ordinal(), 3)];
            args.add("-drive");
            args.add("file=" + vm.diskPath + ",if=virtio,format=" + fmt
                + (vm.diskDiscard ? ",discard=unmap" : ""));
        }

        // SCSI controller + multiqueue
        if (vm.useScsiCtrl) {
            String scsi = "virtio-scsi-pci,id=scsi0";
            if (vm.scsiMultiqueue) scsi += ",num-queues=" + (vm.sockets * vm.cores * vm.threads);
            args.add("-device"); args.add(scsi);
        }

        // ISO
        if (!vm.isoPath.isEmpty()) { args.add("-cdrom"); args.add(vm.isoPath); }

        // Boot
        if (!vm.bootOrder.isEmpty()) {
            String b = "order=" + vm.bootOrder;
            if (vm.bootMenu) b += ",menu=on";
            args.add("-boot"); args.add(b);
        }

        // Display
        String[] dispMap = {"sdl","gtk","spice-app","vnc=:0","egl-headless","none"};
        args.add("-display");
        args.add(dispMap[Math.min(vm.display.ordinal(), dispMap.length - 1)]);

        // GPU
        switch (vm.gpu) {
            case VGA:                   args.add("-device"); args.add("VGA"); break;
            case VirtIO_GPU:            args.add("-device"); args.add("virtio-gpu-pci"); break;
            case VirtIO_GPU_GL:         args.add("-device"); args.add("virtio-gpu-gl-pci"); break;
            case VirtIO_GPU_Rutabaga:   args.add("-device"); args.add("virtio-gpu-rutabaga-pci"); break;
            case VirtIO_GPU_NativeCtx:  args.add("-device"); args.add("virtio-gpu-pci,hostmem=256M"); break;
            case QXL:                   args.add("-device"); args.add("qxl-vga"); break;
            case Cirrus:                args.add("-device"); args.add("cirrus-vga"); break;
            case VMwareSVGA:            args.add("-device"); args.add("vmware-svga"); break;
            case ramfb:                 args.add("-device"); args.add("ramfb"); break;
            case None: break;
        }
        if (vm.virglEnabled) {
            args.add("-device"); args.add("virtio-vga-gl");
        }

        // Audio
        switch (vm.audio) {
            case IntelHDA:
                args.add("-device"); args.add("intel-hda");
                args.add("-device"); args.add("hda-duplex");
                break;
            case AC97:
                args.add("-device"); args.add("AC97");
                break;
            case SB16:
                args.add("-device"); args.add("sb16");
                break;
            case VirtIO_Sound:
                args.add("-device"); args.add("virtio-sound-pci");
                break;
            case None: break;
        }

        // Network
        switch (vm.net) {
            case User:
                args.add("-netdev"); args.add("user,id=n0");
                args.add("-device"); args.add("virtio-net-pci,netdev=n0");
                break;
            case TAP:
                args.add("-netdev"); args.add("tap,id=n0,ifname=tap0,script=no,downscript=no");
                args.add("-device"); args.add("virtio-net-pci,netdev=n0");
                break;
            case Bridge:
                args.add("-netdev"); args.add("bridge,id=n0,br=br0");
                args.add("-device"); args.add("virtio-net-pci,netdev=n0");
                break;
            case Socket:
                args.add("-netdev"); args.add("socket,id=n0,listen=:1234");
                args.add("-device"); args.add("virtio-net-pci,netdev=n0");
                break;
        }

        // IOMMU
        switch (vm.iommu) {
            case Intel:       args.add("-device"); args.add("intel-iommu"); break;
            case SMMUv3:      args.add("-device"); args.add("arm-smmuv3"); break;
            case VirtIO_IOMMU:args.add("-device"); args.add("virtio-iommu-pci"); break;
            default: break;
        }
        if (vm.riscvIommu) { args.add("-device"); args.add("riscv-iommu-sys"); }

        // TPM
        switch (vm.tpm) {
            case TIS: args.add("-chardev"); args.add("socket,id=chrtpm,path=/tmp/mytpm0/swtpm-sock");
                      args.add("-tpmdev");  args.add("emulator,id=tpm0,chardev=chrtpm");
                      args.add("-device");  args.add("tpm-tis,tpmdev=tpm0"); break;
            case CRB: args.add("-chardev"); args.add("socket,id=chrtpm,path=/tmp/mytpm0/swtpm-sock");
                      args.add("-tpmdev");  args.add("emulator,id=tpm0,chardev=chrtpm");
                      args.add("-device");  args.add("tpm-crb,tpmdev=tpm0"); break;
            default: break;
        }

        // Confidential VM (QEMU 11 KVM)
        switch (vm.confVm) {
            case SEV_SNP:
                args.add("-object"); args.add("sev-snp-guest,id=sev0,cbitpos=51,reduced-phys-bits=1");
                args.add("-machine"); args.add(mach + ",confidential-guest-support=sev0");
                break;
            case TDX:
                args.add("-object"); args.add("tdx-guest,id=tdx0");
                args.add("-machine"); args.add(mach + ",confidential-guest-support=tdx0");
                break;
            default: break;
        }

        // Misc flags
        if (vm.snapshotMode) args.add("-snapshot");
        if (vm.noReboot)     { args.add("-no-reboot"); }

        // Extra args
        if (!vm.extraArgs.isEmpty())
            args.addAll(Arrays.asList(vm.extraArgs.trim().split("[ \t]+")));

        return args;
    }

    // ── Format as shell command string ───────────────────────────────
    public static String formatCommand(VMConfig vm) {
        String bin = resolveBinary(vm);
        List<String> args = buildArgs(vm);
        StringBuilder sb = new StringBuilder(bin);
        for (int i = 0; i < args.size(); i++) {
            String a = args.get(i);
            if (a.startsWith("-") && i > 0) sb.append(" \\\n  ");
            else sb.append(" ");
            sb.append(a.contains(" ") ? "\"" + a + "\"" : a);
        }
        return sb.toString();
    }

    // Legacy alias — same as formatCommand
    public static String buildCommand(VMConfig vm) {
        return formatCommand(vm);
    }
}

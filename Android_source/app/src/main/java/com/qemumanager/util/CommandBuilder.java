package com.qemumanager.util;

import com.qemumanager.model.VMConfig;
import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class CommandBuilder {

    // Known QEMU binary search paths — covers native Android, Termux, and PRoot
    private static final String[] BIN_PATHS = {
        "/usr/bin",                               // Standard Linux / PRoot
        "/data/data/com.termux/files/usr/bin",    // Termux native
        "/system/bin",                            // Android system (unlikely but check)
        "/vendor/bin"                             // Android vendor
    };

    private static final String[] SYSTEM_BINS = {
        "qemu-system-x86_64","qemu-system-i386",
        "qemu-system-aarch64","qemu-system-arm","qemu-system-arm",
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

    /** Returns the first existing binary directory, or /usr/bin as fallback. */
    public static String getBinDir() {
        for (String p : BIN_PATHS) {
            File f = new File(p);
            if (f.exists() && f.isDirectory() && f.canRead()) return p;
        }
        return "/usr/bin"; // fallback for PRoot where path may appear later
    }

    /** Returns all existing, readable binary directories. */
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
        return (i >= 0 && i < t.length) ? t[i] : (mode == VMConfig.VMMode.System ? "qemu-system-x86_64" : "qemu-x86_64");
    }

    public static String resolveBinary(VMConfig vm) {
        String name = (vm.binaryMode == VMConfig.BinaryMode.Custom && !vm.customBinary.isEmpty())
            ? vm.customBinary
            : archToQemuBin(vm.arch, vm.mode);
        // Find which bin dir actually has it
        for (String dir : getBinDirs()) {
            File f = new File(dir, name);
            if (f.exists()) return dir + "/" + name;
        }
        // Fallback: use the default bin dir (may not exist yet — PRoot may mount it later)
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
        } catch (Exception e) {
            // SecurityException or other — return empty list
        }
        return out;
    }

    public static String validateCustomBinary(String name) {
        if (name == null || name.isEmpty()) return "Binary name is empty.";
        if (name.contains(" "))  return "Binary name must not contain spaces.";
        if (name.contains("/"))  return "Binary name must not contain slashes (resolved automatically).";
        for (char c : name.toCharArray())
            if (!Character.isLetterOrDigit(c) && c != '-' && c != '_')
                return "Invalid character '" + c + "' in binary name.";
        if (!name.startsWith("qemu-")) return "Binary must start with 'qemu-'.";
        // Check all known bin dirs — binary may be in Termux or PRoot path
        // If not found, don't block creation (user may install QEMU later or use PRoot)
        return "";
    }

    public static List<String> buildArgs(VMConfig vm) {
        List<String> args = new ArrayList<>();

        if (vm.mode == VMConfig.VMMode.UserMode) {
            if (!vm.extraArgs.isEmpty())
                args.addAll(Arrays.asList(vm.extraArgs.trim().split("\\s+")));
            return args;
        }

        // Machine
        String[] machMap = {"pc","q35","virt","microvm",""};
        String mach = (vm.machine == VMConfig.MachineType.custom)
            ? vm.machineCustom : machMap[vm.machine.ordinal()];
        if (mach.isEmpty()) mach = "q35";
        args.add("-machine"); args.add(mach);

        // Accelerator
        switch (vm.accel) {
            case TCG:
                args.add("-accel"); args.add("tcg,thread=multi");
                args.add("-cpu"); args.add(vm.cpuModel);
                break;
            case KVM:
                args.add("-accel"); args.add("kvm");
                args.add("-cpu"); args.add(vm.cpuModel);
                break;
            case KVM_LBT:
                args.add("-accel"); args.add("kvm");
                args.add("-cpu"); args.add(vm.cpuModel + ",lbt=on");
                break;
        }

        args.add("-smp");
        args.add(vm.sockets*vm.cores*vm.threads
            + ",sockets=" + vm.sockets
            + ",cores="   + vm.cores
            + ",threads=" + vm.threads);

        // RAM
        args.add("-m"); args.add(String.valueOf(vm.ramMb));
        if (vm.ballooning) { args.add("-device"); args.add("virtio-balloon-pci"); }

        // UEFI
        if (vm.uefi) { args.add("-bios"); args.add("/usr/share/ovmf/OVMF.fd"); }

        // Disk
        if (!vm.diskPath.isEmpty()) {
            String[] fmts = {"qcow2","raw","vmdk","vdi"};
            args.add("-drive");
            args.add("file=" + vm.diskPath + ",if=virtio,format=" + fmts[vm.diskFormat.ordinal()]);
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
        args.add("-display"); args.add(dispMap[vm.display.ordinal()]);

        // GPU
        String[] gpuMap = {"VGA","virtio-gpu-pci","qxl-vga","cirrus-vga","vmware-svga",""};
        if (vm.gpu != VMConfig.GPUType.None) {
            args.add("-device"); args.add(gpuMap[vm.gpu.ordinal()]);
        }

        // Audio
        switch (vm.audio) {
            case IntelHDA:
                args.add("-device"); args.add("intel-hda");
                args.add("-device"); args.add("hda-duplex"); break;
            case AC97:
                args.add("-device"); args.add("AC97"); break;
            case SB16:
                args.add("-device"); args.add("sb16"); break;
            default: break;
        }

        // USB
        args.add("-usb");

        // Network
        switch (vm.net) {
            case User:
                args.add("-netdev"); args.add("user,id=net0");
                args.add("-device"); args.add("virtio-net-pci,netdev=net0"); break;
            case TAP:
                args.add("-netdev"); args.add("tap,id=net0,ifname=tap0,script=no");
                args.add("-device"); args.add("virtio-net-pci,netdev=net0"); break;
            case Bridge:
                args.add("-netdev"); args.add("bridge,id=net0,br=br0");
                args.add("-device"); args.add("virtio-net-pci,netdev=net0"); break;
            case Socket:
                args.add("-netdev"); args.add("socket,id=net0,listen=:4444");
                args.add("-device"); args.add("virtio-net-pci,netdev=net0"); break;
        }

        // Monitor socket
        if (!vm.vmDir.isEmpty()) {
            args.add("-monitor"); args.add("unix:" + vm.vmDir + "/monitor.sock,server,nowait");
            args.add("-pidfile");  args.add(vm.vmDir + "/vm.pid");
        }

        // Extra args LAST
        if (!vm.extraArgs.isEmpty())
            args.addAll(Arrays.asList(vm.extraArgs.trim().split("\\s+")));

        return args;
    }

    public static String buildCommand(VMConfig vm) {
        String bin = resolveBinary(vm);
        StringBuilder sb = new StringBuilder(bin);
        for (String a : buildArgs(vm)) sb.append(" ").append(a);
        return sb.toString();
    }

    public static String formatCommand(VMConfig vm) {
        String bin = resolveBinary(vm);
        List<String> args = buildArgs(vm);
        StringBuilder sb = new StringBuilder(bin);
        for (int i = 0; i < args.size(); i++) {
            if (i % 2 == 0) sb.append(" \\\n  ").append(args.get(i));
            else             sb.append(" ").append(args.get(i));
        }
        return sb.toString();
    }
}

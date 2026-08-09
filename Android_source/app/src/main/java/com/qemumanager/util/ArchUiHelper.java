package com.qemumanager.util;

import android.app.Activity;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import com.qemumanager.R;
import com.qemumanager.model.VMConfig;

/**
 * Shared arch-driven dropdown logic used by both the VM creation wizard and
 * the VM settings editor, so machine/CPU/GPU/audio/accelerator choices are
 * always filtered to what's actually valid for the selected architecture in
 * both places.
 */
public final class ArchUiHelper {
    private ArchUiHelper() {}

    public enum ArchGroup { X86, I386, AARCH64, ARM, RISCV64, RISCV32, MIPS, PPC, S390, LOONG, GENERIC }

    public static ArchGroup archGroup(int pos) {
        switch (pos) {
            case 0:  return ArchGroup.X86;
            case 1:  return ArchGroup.I386;
            case 2:  return ArchGroup.AARCH64;
            case 3: case 4: return ArchGroup.ARM;
            case 5:  return ArchGroup.RISCV64;
            case 6:  return ArchGroup.RISCV32;
            case 7: case 8: case 9: case 10: case 11: case 12: return ArchGroup.MIPS;
            case 13: case 14: case 15: return ArchGroup.PPC;
            case 26: return ArchGroup.S390;
            case 25: return ArchGroup.LOONG;
            default: return ArchGroup.GENERIC;
        }
    }

    public static int machArr(ArchGroup g) {
        switch (g) {
            case X86: case I386: return R.array.machine_x86;
            case AARCH64: case ARM: return R.array.machine_arm;
            case RISCV64: case RISCV32: return R.array.machine_riscv;
            case MIPS: return R.array.machine_mips;
            case PPC: return R.array.machine_ppc;
            case S390: return R.array.machine_s390;
            case LOONG: return R.array.machine_loong;
            default: return R.array.machine_generic;
        }
    }

    public static int cpuArr(ArchGroup g) {
        switch (g) {
            case X86: return R.array.cpu_x86_64;
            case I386: return R.array.cpu_i386;
            case AARCH64: return R.array.cpu_aarch64;
            case ARM: return R.array.cpu_arm;
            case RISCV64: return R.array.cpu_riscv64;
            case RISCV32: return R.array.cpu_riscv32;
            case MIPS: return R.array.cpu_mips;
            case PPC: return R.array.cpu_ppc;
            case S390: return R.array.cpu_s390;
            case LOONG: return R.array.cpu_loong;
            default: return R.array.cpu_generic;
        }
    }

    public static int gpuArr(ArchGroup g) {
        switch (g) {
            case X86: case I386: return R.array.gpu_x86;
            case AARCH64: case ARM: return R.array.gpu_arm;
            default: return R.array.gpu_minimal;
        }
    }

    public static int audioArr(ArchGroup g) {
        switch (g) {
            case X86: case I386: return R.array.audio_x86;
            case AARCH64: case ARM: return R.array.audio_arm;
            default: return R.array.audio_generic;
        }
    }

    public static int accelArr(ArchGroup g) {
        switch (g) {
            case X86: case I386: return R.array.accel_x86;
            case AARCH64: case ARM: return R.array.accel_arm;
            default: return R.array.accel_generic;
        }
    }

    public static void populateSpinner(Activity ctx, Spinner sp, int resId) {
        if (sp == null) return;
        String[] items = ctx.getResources().getStringArray(resId);
        ArrayAdapter<String> ad = new ArrayAdapter<>(ctx, android.R.layout.simple_spinner_item, items);
        ad.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        sp.setAdapter(ad);
    }

    // ── Forward parse: spinner label -> enum ───────────────────────────

    public static VMConfig.MachineType parseMachine(String m) {
        if (m == null) m = "";
        if      (m.startsWith("q35"))         return VMConfig.MachineType.q35;
        else if (m.startsWith("pc"))          return VMConfig.MachineType.pc;
        else if (m.startsWith("virt-acpi"))   return VMConfig.MachineType.virt_acpi;
        else if (m.startsWith("virt"))        return VMConfig.MachineType.virt;
        else if (m.startsWith("microvm"))     return VMConfig.MachineType.microvm;
        else if (m.startsWith("x86-microvm")) return VMConfig.MachineType.x86_64_microvm;
        else if (m.startsWith("nitro"))       return VMConfig.MachineType.nitro_enclave;
        else if (m.startsWith("sbsa"))        return VMConfig.MachineType.sbsa_ref;
        else                                  return VMConfig.MachineType.custom;
    }

    public static String parseCpuModel(String raw) {
        if (raw == null) return "max";
        int p = raw.indexOf(" (");
        return p >= 0 ? raw.substring(0, p).trim() : raw.trim();
    }

    public static VMConfig.GPUType parseGpu(String g) {
        if (g == null) g = "";
        if      (g.contains("GL"))          return VMConfig.GPUType.VirtIO_GPU_GL;
        else if (g.contains("Rutabaga"))    return VMConfig.GPUType.VirtIO_GPU_Rutabaga;
        else if (g.contains("NativeCtx"))   return VMConfig.GPUType.VirtIO_GPU_NativeCtx;
        else if (g.contains("VirtIO"))      return VMConfig.GPUType.VirtIO_GPU;
        else if (g.contains("QXL"))         return VMConfig.GPUType.QXL;
        else if (g.contains("VMware"))      return VMConfig.GPUType.VMwareSVGA;
        else if (g.contains("Cirrus"))      return VMConfig.GPUType.Cirrus;
        else if (g.contains("ramfb"))       return VMConfig.GPUType.ramfb;
        else if (g.contains("None"))        return VMConfig.GPUType.None;
        else                                return VMConfig.GPUType.VGA;
    }

    public static VMConfig.AudioType parseAudio(String a) {
        if (a == null) a = "";
        if      (a.contains("Intel HDA")) return VMConfig.AudioType.IntelHDA;
        else if (a.contains("AC97"))      return VMConfig.AudioType.AC97;
        else if (a.contains("SB16"))      return VMConfig.AudioType.SB16;
        else if (a.contains("VirtIO"))    return VMConfig.AudioType.VirtIO_Sound;
        else                              return VMConfig.AudioType.None;
    }

    public static VMConfig.Accelerator parseAccel(String a) {
        if (a == null) a = "";
        if      (a.startsWith("KVM + LBT")) return VMConfig.Accelerator.KVM_LBT;
        else if (a.startsWith("KVM"))       return VMConfig.Accelerator.KVM;
        else if (a.startsWith("WHPX"))      return VMConfig.Accelerator.WHPX;
        else if (a.startsWith("HVF"))       return VMConfig.Accelerator.HVF;
        else if (a.startsWith("Nitro"))     return VMConfig.Accelerator.Nitro;
        else if (a.startsWith("MSHV"))      return VMConfig.Accelerator.MSHV;
        else                                 return VMConfig.Accelerator.TCG;
    }

    // ── Reverse match: does this spinner label correspond to enum value? ──
    // Used to pre-select the right item when editing an existing VM, since
    // each arch's array has different text/ordering.

    public static boolean machineMatches(String label, VMConfig.MachineType t) {
        return parseMachine(label) == t;
    }

    public static boolean gpuMatches(String label, VMConfig.GPUType t) {
        return parseGpu(label) == t;
    }

    public static boolean audioMatches(String label, VMConfig.AudioType t) {
        return parseAudio(label) == t;
    }

    public static boolean accelMatches(String label, VMConfig.Accelerator t) {
        return parseAccel(label) == t;
    }

    /** Selects the first spinner item for which pred(label) is true. Leaves selection at 0 if none match. */
    public static void selectMatching(Spinner sp, java.util.function.Predicate<String> pred) {
        if (sp == null || sp.getAdapter() == null) return;
        int count = sp.getAdapter().getCount();
        for (int i = 0; i < count; i++) {
            Object item = sp.getAdapter().getItem(i);
            if (item instanceof String && pred.test((String) item)) {
                sp.setSelection(i);
                return;
            }
        }
    }
}

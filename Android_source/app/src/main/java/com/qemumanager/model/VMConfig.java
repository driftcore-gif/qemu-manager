package com.qemumanager.model;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import java.util.ArrayList;
import java.util.List;

public class VMConfig {

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
    public enum MachineType { pc, q35, virt, microvm, custom }
    public enum DisplayType { SDL, GTK, SPICE, VNC, EGL, Headless }
    public enum NetworkMode { User, TAP, Bridge, Socket }
    public enum DiskFormat  { qcow2, raw, vmdk, vdi }
    public enum GPUType     { VGA, VirtIO, QXL, Cirrus, VMwareSVGA, None }
    public enum AudioType   { IntelHDA, AC97, SB16, None }
    public enum Accelerator { TCG, KVM, KVM_LBT }
    public enum BinaryMode  { Auto, Custom }

    // Identity
    public String name        = "";
    public String description = "";

    // Architecture
    public VMArch      arch    = VMArch.x86_64;
    public VMMode      mode    = VMMode.System;
    public MachineType machine = MachineType.q35;
    public String      machineCustom = "";

    // CPU
    public String cpuModel = "max";
    public int    sockets  = 1;
    public int    cores    = 2;
    public int    threads  = 2;

    // Memory
    public int     ramMb      = 2048;
    public boolean ballooning = false;

    // Storage
    public String     diskPath   = "";
    public String     isoPath    = "";
    public DiskFormat diskFormat = DiskFormat.qcow2;
    public int        diskSizeGb = 20;

    // Boot
    public boolean uefi      = false;
    public boolean bootMenu  = false;
    public String  bootOrder = "cd";

    // Display
    public DisplayType display    = DisplayType.SDL;
    public GPUType     gpu        = GPUType.VirtIO;
    public AudioType   audio      = AudioType.IntelHDA;

    // Network
    public NetworkMode net = NetworkMode.User;

    // Binary
    public BinaryMode binaryMode   = BinaryMode.Auto;
    public String     customBinary = "";

    // Accelerator
    public Accelerator accel = Accelerator.TCG;

    // Extra args
    public String  extraArgs          = "";
    public boolean saveScriptToFolder = false;

    // Runtime
    public VMStatus status      = VMStatus.Stopped;
    public String   vmDir       = "";
    public String   lastStarted = "";

    // ----------------------------------------------------------------
    public JSONObject toJson() throws JSONException {
        JSONObject o = new JSONObject();
        o.put("name",        name);
        o.put("description", description);
        o.put("arch",        arch.name());
        o.put("mode",        mode.name());
        o.put("machine",     machine.name());
        o.put("machine_custom", machineCustom);
        o.put("cpu_model",   cpuModel);
        o.put("sockets",     sockets);
        o.put("cores",       cores);
        o.put("threads",     threads);
        o.put("ram_mb",      ramMb);
        o.put("ballooning",  ballooning);
        o.put("disk_path",   diskPath);
        o.put("iso_path",    isoPath);
        o.put("disk_format", diskFormat.name());
        o.put("disk_size_gb",diskSizeGb);
        o.put("uefi",        uefi);
        o.put("boot_menu",   bootMenu);
        o.put("boot_order",  bootOrder);
        o.put("display",     display.name());
        o.put("gpu",         gpu.name());
        o.put("audio",       audio.name());
        o.put("net_mode",    net.name());
        o.put("binary_mode", binaryMode.name());
        o.put("custom_binary", customBinary);
        o.put("accel",       accel.name());
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
        try { c.arch    = VMArch.valueOf(o.optString("arch","x86_64")); } catch(Exception ignored){}
        try { c.mode    = VMMode.valueOf(o.optString("mode","System")); } catch(Exception ignored){}
        try { c.machine = MachineType.valueOf(o.optString("machine","q35")); } catch(Exception ignored){}
        c.machineCustom = o.optString("machine_custom","");
        c.cpuModel      = o.optString("cpu_model","max");
        c.sockets       = o.optInt("sockets",1);
        c.cores         = o.optInt("cores",2);
        c.threads       = o.optInt("threads",2);
        c.ramMb         = o.optInt("ram_mb",2048);
        c.ballooning    = o.optBoolean("ballooning",false);
        c.diskPath      = o.optString("disk_path","");
        c.isoPath       = o.optString("iso_path","");
        try { c.diskFormat = DiskFormat.valueOf(o.optString("disk_format","qcow2")); } catch(Exception ignored){}
        c.diskSizeGb    = o.optInt("disk_size_gb",20);
        c.uefi          = o.optBoolean("uefi",false);
        c.bootMenu      = o.optBoolean("boot_menu",false);
        c.bootOrder     = o.optString("boot_order","cd");
        try { c.display = DisplayType.valueOf(o.optString("display","SDL")); } catch(Exception ignored){}
        try { c.gpu     = GPUType.valueOf(o.optString("gpu","VirtIO")); } catch(Exception ignored){}
        try { c.audio   = AudioType.valueOf(o.optString("audio","IntelHDA")); } catch(Exception ignored){}
        try { c.net     = NetworkMode.valueOf(o.optString("net_mode","User")); } catch(Exception ignored){}
        try { c.binaryMode = BinaryMode.valueOf(o.optString("binary_mode","Auto")); } catch(Exception ignored){}
        c.customBinary  = o.optString("custom_binary","");
        try { c.accel   = Accelerator.valueOf(o.optString("accel","TCG")); } catch(Exception ignored){}
        c.extraArgs     = o.optString("extra_args","");
        c.saveScriptToFolder = o.optBoolean("save_script_to_vm_folder",false);
        try { c.status  = VMStatus.valueOf(o.optString("status","Stopped")); } catch(Exception ignored){}
        c.vmDir         = o.optString("vm_dir","");
        c.lastStarted   = o.optString("last_started","");
        return c;
    }
}

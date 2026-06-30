#include "vmconfig_io.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <map>

// -------------------------------------------------------
// Minimal JSON helpers  (no external lib)
// -------------------------------------------------------
static std::string jstr(const std::string& s) {
    std::string r = "\"";
    for (char c : s) {
        if      (c == '"')  r += "\\\"";
        else if (c == '\\') r += "\\\\";
        else if (c == '\n') r += "\\n";
        else                r += c;
    }
    r += "\"";
    return r;
}

static std::string jbool(bool b)  { return b ? "true" : "false"; }
static std::string jint(int v)    { return std::to_string(v); }

// Extract string value for key from flat JSON (no nesting)
static std::string jget(const std::string& j, const std::string& key) {
    std::regex re("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch m;
    if (std::regex_search(j, m, re)) return m[1].str();
    return {};
}
static int jgeti(const std::string& j, const std::string& key, int def=0) {
    std::regex re("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    std::smatch m;
    if (std::regex_search(j, m, re)) return std::stoi(m[1].str());
    return def;
}
static bool jgetb(const std::string& j, const std::string& key, bool def=false) {
    std::regex re("\"" + key + "\"\\s*:\\s*(true|false)");
    std::smatch m;
    if (std::regex_search(j, m, re)) return m[1].str() == "true";
    return def;
}

// -------------------------------------------------------
// Enum -> string
// -------------------------------------------------------
static std::string archStr(VMArch a) {
    const char* t[] = {
        "x86_64","i386","aarch64","arm","armeb",
        "riscv64","riscv32",
        "mips","mipsel","mips64","mips64el","mipsn32","mipsn32el",
        "ppc","ppc64","ppc64le",
        "sparc","sparc64","sparc32plus",
        "alpha","hppa","m68k",
        "microblaze","microblazeel",
        "or1k","loongarch64","s390x",
        "sh4","sh4eb","xtensa","xtensaeb",
        "tricore","rx","avr","hexagon"
    };
    return t[(int)a];
}
static VMArch archFrom(const std::string& s) {
    const char* t[] = {
        "x86_64","i386","aarch64","arm","armeb",
        "riscv64","riscv32",
        "mips","mipsel","mips64","mips64el","mipsn32","mipsn32el",
        "ppc","ppc64","ppc64le",
        "sparc","sparc64","sparc32plus",
        "alpha","hppa","m68k",
        "microblaze","microblazeel",
        "or1k","loongarch64","s390x",
        "sh4","sh4eb","xtensa","xtensaeb",
        "tricore","rx","avr","hexagon",nullptr
    };
    for (int i = 0; t[i]; i++) if (s == t[i]) return (VMArch)i;
    return VMArch::x86_64;
}

#define ESTR(e, ...) static std::string e##Str(decltype(VMConfig::e) v) {     const char* t[] = {__VA_ARGS__}; return t[(int)v]; }     static decltype(VMConfig::e) e##From(const std::string& s) {     const char* t[] = {__VA_ARGS__};     for (int i=0;t[i];i++) if(s==t[i]) return (decltype(VMConfig::e))i;     return (decltype(VMConfig::e))0; }

ESTR(mode,    "System","UserMode",nullptr)
ESTR(machine, "pc","q35","virt","microvm","custom",nullptr)
ESTR(display, "GTK","SDL","SPICE","VNC","EGL","Headless",nullptr)
ESTR(net_mode,"User","TAP","Bridge","Socket",nullptr)
ESTR(disk_format,"qcow2","raw","vmdk","vdi",nullptr)
ESTR(gpu,     "VGA","VirtIO","QXL","Cirrus","VMwareSVGA","None",nullptr)
ESTR(audio,   "IntelHDA","AC97","SB16","None",nullptr)

// -------------------------------------------------------

static std::string accelStr(Accelerator a) {
    const char* t[]={"TCG","KVM","KVM_LBT",nullptr};
    return t[(int)a];
}
static Accelerator accelFrom(const std::string& s) {
    const char* t[]={"TCG","KVM","KVM_LBT",nullptr};
    for(int i=0;t[i];i++) if(s==t[i]) return (Accelerator)i;
    return Accelerator::TCG;
}
static std::string binaryModeStr(BinaryMode m) {
    return m==BinaryMode::Custom ? "Custom" : "Auto";
}
static BinaryMode binaryModeFrom(const std::string& s) {
    return s=="Custom" ? BinaryMode::Custom : BinaryMode::Auto;
}

std::string VMConfigIO::toJson(const VMConfig& c) {
    std::ostringstream o;
    o << "{\n";
    o << "  \"name\":"         << jstr(c.name)            << ",\n";
    o << "  \"description\":"  << jstr(c.description)     << ",\n";
    o << "  \"vm_dir\":"       << jstr(c.vm_dir)          << ",\n";
    o << "  \"arch\":"         << jstr(archStr(c.arch))   << ",\n";
    o << "  \"mode\":"         << jstr(modeStr(c.mode))   << ",\n";
    o << "  \"machine\":"      << jstr(machineStr(c.machine)) << ",\n";
    o << "  \"machine_custom\":"<< jstr(c.machine_custom) << ",\n";
    o << "  \"cpu_model\":"    << jstr(c.cpu_model)       << ",\n";
    o << "  \"sockets\":"      << jint(c.sockets)         << ",\n";
    o << "  \"cores\":"        << jint(c.cores)           << ",\n";
    o << "  \"threads\":"      << jint(c.threads)         << ",\n";
    o << "  \"ram_mb\":"       << jint(c.ram_mb)          << ",\n";
    o << "  \"ballooning\":"   << jbool(c.ballooning)     << ",\n";
    o << "  \"disk_path\":"    << jstr(c.disk_path)       << ",\n";
    o << "  \"disk_format\":"  << jstr(disk_formatStr(c.disk_format)) << ",\n";
    o << "  \"disk_size_gb\":" << jint(c.disk_size_gb)   << ",\n";
    o << "  \"iso_path\":"     << jstr(c.iso_path)        << ",\n";
    o << "  \"uefi\":"         << jbool(c.uefi)           << ",\n";
    o << "  \"secure_boot\":"  << jbool(c.secure_boot)    << ",\n";
    o << "  \"boot_menu\":"    << jbool(c.boot_menu)      << ",\n";
    o << "  \"boot_order\":"   << jstr(c.boot_order)      << ",\n";
    o << "  \"display\":"      << jstr(displayStr(c.display)) << ",\n";
    o << "  \"gpu\":"          << jstr(gpuStr(c.gpu))     << ",\n";
    o << "  \"audio\":"        << jstr(audioStr(c.audio)) << ",\n";
    o << "  \"net_mode\":"     << jstr(net_modeStr(c.net_mode)) << ",\n";
    o << "  \"extra_args\":"   << jstr(c.extra_args)      << ",\n";
    o << "  \"binary_mode\":" << jstr(binaryModeStr(c.binary_mode)) << ",\n";
    o << "  \"custom_binary\":" << jstr(c.custom_binary) << ",\n";
    o << "  \"accel\":" << jstr(accelStr(c.accel)) << ",\n";
    o << "  \"save_script_to_vm_folder\":" << jbool(c.save_script_to_vm_folder) << ",\n";
    o << "  \"status\":"       << jstr("Stopped")         << ",\n";
    o << "  \"last_started\":" << jstr(c.last_started)    << "\n";
    o << "}\n";
    return o.str();
}

bool VMConfigIO::fromJson(const std::string& j, VMConfig& c) {
    if (j.empty()) return false;
    c.name         = jget(j, "name");
    c.description  = jget(j, "description");
    c.vm_dir       = jget(j, "vm_dir");
    c.arch         = archFrom(jget(j, "arch"));
    c.mode         = modeFrom(jget(j, "mode"));
    c.machine      = machineFrom(jget(j, "machine"));
    c.machine_custom = jget(j, "machine_custom");
    c.cpu_model    = jget(j, "cpu_model");
    c.sockets      = jgeti(j, "sockets", 1);
    c.cores        = jgeti(j, "cores", 2);
    c.threads      = jgeti(j, "threads", 2);
    c.ram_mb       = jgeti(j, "ram_mb", 2048);
    c.ballooning   = jgetb(j, "ballooning");
    c.disk_path    = jget(j, "disk_path");
    c.disk_format  = disk_formatFrom(jget(j, "disk_format"));
    c.disk_size_gb = jgeti(j, "disk_size_gb", 20);
    c.iso_path     = jget(j, "iso_path");
    c.uefi         = jgetb(j, "uefi");
    c.secure_boot  = jgetb(j, "secure_boot");
    c.boot_menu    = jgetb(j, "boot_menu");
    c.boot_order   = jget(j, "boot_order");
    c.display      = displayFrom(jget(j, "display"));
    c.gpu          = gpuFrom(jget(j, "gpu"));
    c.audio        = audioFrom(jget(j, "audio"));
    c.net_mode     = net_modeFrom(jget(j, "net_mode"));
    c.extra_args   = jget(j, "extra_args");
    c.binary_mode   = binaryModeFrom(jget(j, "binary_mode"));
    c.custom_binary = jget(j, "custom_binary");
    c.accel         = accelFrom(jget(j, "accel"));
    c.save_script_to_vm_folder = jgetb(j, "save_script_to_vm_folder");
    c.last_started = jget(j, "last_started");
    return !c.name.empty();
}

bool VMConfigIO::save(const VMConfig& c) {
    std::ofstream f(c.vm_dir + "/vm.json");
    if (!f) return false;
    f << toJson(c);
    return true;
}

bool VMConfigIO::load(const std::string& vm_dir, VMConfig& out) {
    std::ifstream f(vm_dir + "/vm.json");
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    return fromJson(ss.str(), out);
}

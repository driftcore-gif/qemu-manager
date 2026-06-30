#include "vm.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <sys/stat.h>

// Simple JSON serialization without external deps
static std::string escapeJson(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

static std::string archStr(VMArch a) {
    switch(a) {
        case VMArch::x86_64: return "x86_64";
        case VMArch::i386: return "i386";
        case VMArch::aarch64: return "aarch64";
        case VMArch::arm: return "arm";
        case VMArch::riscv64: return "riscv64";
        case VMArch::riscv32: return "riscv32";
        case VMArch::mips: return "mips";
        case VMArch::mipsel: return "mipsel";
        case VMArch::ppc64: return "ppc64";
        case VMArch::sparc: return "sparc";
        default: return "x86_64";
    }
}

static std::string machineStr(MachineType m) {
    switch(m) {
        case MachineType::pc: return "pc";
        case MachineType::q35: return "q35";
        case MachineType::virt: return "virt";
        case MachineType::microvm: return "microvm";
        default: return "q35";
    }
}

static std::string diskFmtStr(DiskFormat f) {
    switch(f) {
        case DiskFormat::qcow2: return "qcow2";
        case DiskFormat::raw: return "raw";
        case DiskFormat::vmdk: return "vmdk";
        case DiskFormat::vdi: return "vdi";
        default: return "qcow2";
    }
}

std::string vmToJson(const VMConfig& vm) {
    std::ostringstream j;
    j << "{\n";
    j << "  \"name\": \"" << escapeJson(vm.name) << "\",\n";
    j << "  \"description\": \"" << escapeJson(vm.description) << "\",\n";
    j << "  \"arch\": \"" << archStr(vm.arch) << "\",\n";
    j << "  \"machine\": \"" << machineStr(vm.machine) << "\",\n";
    j << "  \"cpu_model\": \"" << escapeJson(vm.cpu_model) << "\",\n";
    j << "  \"sockets\": " << vm.sockets << ",\n";
    j << "  \"cores\": " << vm.cores << ",\n";
    j << "  \"threads\": " << vm.threads << ",\n";
    j << "  \"ram_mb\": " << vm.ram_mb << ",\n";
    j << "  \"disk_path\": \"" << escapeJson(vm.disk_path) << "\",\n";
    j << "  \"disk_format\": \"" << diskFmtStr(vm.disk_format) << "\",\n";
    j << "  \"disk_size_gb\": " << vm.disk_size_gb << ",\n";
    j << "  \"iso_path\": \"" << escapeJson(vm.iso_path) << "\",\n";
    j << "  \"uefi\": " << (vm.uefi ? "true" : "false") << ",\n";
    j << "  \"secure_boot\": " << (vm.secure_boot ? "true" : "false") << ",\n";
    j << "  \"boot_order\": \"" << escapeJson(vm.boot_order) << "\",\n";
    j << "  \"boot_menu\": " << (vm.boot_menu ? "true" : "false") << ",\n";
    j << "  \"display\": " << (int)vm.display << ",\n";
    j << "  \"gpu\": " << (int)vm.gpu << ",\n";
    j << "  \"audio\": " << (int)vm.audio << ",\n";
    j << "  \"net_mode\": " << (int)vm.net_mode << ",\n";
    j << "  \"extra_args\": \"" << escapeJson(vm.extra_args) << "\",\n";
    j << "  \"last_started\": \"" << escapeJson(vm.last_started) << "\"\n";
    j << "}\n";
    return j.str();
}

static std::string getJsonVal(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        size_t start = pos + 1;
        size_t end = json.find('"', start);
        return json.substr(start, end - start);
    } else {
        size_t end = json.find_first_of(",\n}", pos);
        return json.substr(pos, end - pos);
    }
}

VMConfig vmFromJson(const std::string& json, const std::string& vm_dir) {
    VMConfig vm;
    vm.vm_dir = vm_dir;
    vm.name = getJsonVal(json, "name");
    vm.description = getJsonVal(json, "description");
    vm.cpu_model = getJsonVal(json, "cpu_model");
    vm.disk_path = getJsonVal(json, "disk_path");
    vm.iso_path = getJsonVal(json, "iso_path");
    vm.boot_order = getJsonVal(json, "boot_order");
    vm.extra_args = getJsonVal(json, "extra_args");
    vm.last_started = getJsonVal(json, "last_started");

    std::string arch = getJsonVal(json, "arch");
    if (arch == "i386") vm.arch = VMArch::i386;
    else if (arch == "aarch64") vm.arch = VMArch::aarch64;
    else if (arch == "arm") vm.arch = VMArch::arm;
    else if (arch == "riscv64") vm.arch = VMArch::riscv64;
    else if (arch == "riscv32") vm.arch = VMArch::riscv32;
    else if (arch == "mips") vm.arch = VMArch::mips;
    else if (arch == "mipsel") vm.arch = VMArch::mipsel;
    else if (arch == "ppc64") vm.arch = VMArch::ppc64;
    else if (arch == "sparc") vm.arch = VMArch::sparc;
    else vm.arch = VMArch::x86_64;

    std::string mach = getJsonVal(json, "machine");
    if (mach == "pc") vm.machine = MachineType::pc;
    else if (mach == "virt") vm.machine = MachineType::virt;
    else if (mach == "microvm") vm.machine = MachineType::microvm;
    else vm.machine = MachineType::q35;

    try { vm.sockets = std::stoi(getJsonVal(json, "sockets")); } catch(...) {}
    try { vm.cores = std::stoi(getJsonVal(json, "cores")); } catch(...) {}
    try { vm.threads = std::stoi(getJsonVal(json, "threads")); } catch(...) {}
    try { vm.ram_mb = std::stoi(getJsonVal(json, "ram_mb")); } catch(...) {}
    try { vm.disk_size_gb = std::stoi(getJsonVal(json, "disk_size_gb")); } catch(...) {}
    try { vm.display = (DisplayType)std::stoi(getJsonVal(json, "display")); } catch(...) {}
    try { vm.gpu = (GPUType)std::stoi(getJsonVal(json, "gpu")); } catch(...) {}
    try { vm.audio = (AudioType)std::stoi(getJsonVal(json, "audio")); } catch(...) {}
    try { vm.net_mode = (NetworkMode)std::stoi(getJsonVal(json, "net_mode")); } catch(...) {}

    std::string uefi_s = getJsonVal(json, "uefi");
    vm.uefi = (uefi_s == "true");
    std::string sb_s = getJsonVal(json, "secure_boot");
    vm.secure_boot = (sb_s == "true");
    std::string bm_s = getJsonVal(json, "boot_menu");
    vm.boot_menu = (bm_s == "true");

    return vm;
}

bool saveVMConfig(const VMConfig& vm) {
    std::string path = vm.vm_dir + "/vm.json";
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << vmToJson(vm);
    return true;
}

VMConfig loadVMConfig(const std::string& vm_dir) {
    std::string path = vm_dir + "/vm.json";
    std::ifstream f(path);
    std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return vmFromJson(json, vm_dir);
}

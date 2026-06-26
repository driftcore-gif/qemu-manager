#include "templates.h"
#include <vector>
#include <string>

std::vector<VMTemplate> getTemplates() {
    return {
        {"Ubuntu 24.04", "ubuntu", VMArch::x86_64, MachineType::q35, "max", 2, 2048, 25, true, false, DisplayType::GTK, GPUType::VirtIO, AudioType::IntelHDA, NetworkMode::User},
        {"Debian 12",    "debian", VMArch::x86_64, MachineType::q35, "max", 2, 1024, 20, false, false, DisplayType::GTK, GPUType::VirtIO, AudioType::IntelHDA, NetworkMode::User},
        {"Fedora 40",    "fedora", VMArch::x86_64, MachineType::q35, "max", 2, 2048, 25, true, false, DisplayType::GTK, GPUType::VirtIO, AudioType::IntelHDA, NetworkMode::User},
        {"Arch Linux",   "arch",   VMArch::x86_64, MachineType::q35, "max", 2, 2048, 20, false, false, DisplayType::GTK, GPUType::VirtIO, AudioType::IntelHDA, NetworkMode::User},
        {"Alpine Linux", "alpine", VMArch::x86_64, MachineType::q35, "host", 1, 512, 8, false, false, DisplayType::GTK, GPUType::VGA, AudioType::None, NetworkMode::User},
        {"Tiny Core",    "tinycore",VMArch::x86_64, MachineType::pc,  "host", 1, 256, 4, false, false, DisplayType::GTK, GPUType::VGA, AudioType::None, NetworkMode::User},
        {"Windows 11",   "windows",VMArch::x86_64, MachineType::q35, "host", 4, 4096, 60, true, true, DisplayType::GTK, GPUType::QXL, AudioType::IntelHDA, NetworkMode::User},
        {"Android x86",  "android",VMArch::x86_64, MachineType::pc,  "host", 2, 2048, 16, false, false, DisplayType::GTK, GPUType::VGA, AudioType::AC97, NetworkMode::User},
        {"ARM64 Debian", "debian_arm", VMArch::aarch64, MachineType::virt, "max", 2, 1024, 16, true, false, DisplayType::GTK, GPUType::VirtIO, AudioType::None, NetworkMode::User},
        {"RISC-V Linux", "riscv",  VMArch::riscv64, MachineType::virt, "rv64", 2, 1024, 16, false, false, DisplayType::GTK, GPUType::VirtIO, AudioType::None, NetworkMode::User},
    };
}

VMConfig templateToConfig(const VMTemplate& t, const std::string& vm_dir) {
    VMConfig vm;
    vm.name = t.name;
    vm.arch = t.arch;
    vm.machine = t.machine;
    vm.cpu_model = t.cpu_model;
    vm.cores = t.cores;
    vm.ram_mb = t.ram_mb;
    vm.disk_size_gb = t.disk_size_gb;
    vm.uefi = t.uefi;
    vm.secure_boot = t.secure_boot;
    vm.display = t.display;
    vm.gpu = t.gpu;
    vm.audio = t.audio;
    vm.net_mode = t.net_mode;
    vm.disk_format = DiskFormat::qcow2;
    vm.vm_dir = vm_dir;
    vm.disk_path = vm_dir + "/disk.qcow2";
    return vm;
}

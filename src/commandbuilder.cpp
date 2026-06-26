#include "commandbuilder.h"
#include <sstream>

std::string CommandBuilder::archToQemuBin(VMArch arch) {
    switch(arch) {
        case VMArch::x86_64:  return "qemu-system-x86_64";
        case VMArch::i386:    return "qemu-system-i386";
        case VMArch::aarch64: return "qemu-system-aarch64";
        case VMArch::arm:     return "qemu-system-arm";
        case VMArch::riscv64: return "qemu-system-riscv64";
        case VMArch::riscv32: return "qemu-system-riscv32";
        case VMArch::mips:    return "qemu-system-mips";
        case VMArch::mipsel:  return "qemu-system-mipsel";
        case VMArch::ppc64:   return "qemu-system-ppc64";
        case VMArch::sparc:   return "qemu-system-sparc";
        default:              return "qemu-system-x86_64";
    }
}

std::vector<std::string> CommandBuilder::buildArgs(const VMConfig& vm) {
    std::vector<std::string> args;

    // Machine
    std::string machine_str;
    switch(vm.machine) {
        case MachineType::pc:     machine_str = "pc"; break;
        case MachineType::q35:    machine_str = "q35"; break;
        case MachineType::virt:   machine_str = "virt"; break;
        case MachineType::microvm:machine_str = "microvm"; break;
    }
    args.push_back("-machine"); args.push_back(machine_str);

    // CPU
    std::string smp = std::to_string(vm.sockets * vm.cores * vm.threads) +
        ",sockets=" + std::to_string(vm.sockets) +
        ",cores=" + std::to_string(vm.cores) +
        ",threads=" + std::to_string(vm.threads);
    args.push_back("-cpu"); args.push_back(vm.cpu_model);
    args.push_back("-smp"); args.push_back(smp);

    // Memory
    args.push_back("-m"); args.push_back(std::to_string(vm.ram_mb));
    if (vm.ballooning) {
        args.push_back("-device"); args.push_back("virtio-balloon-pci");
    }

    // UEFI / BIOS
    if (vm.uefi) {
        // Try common OVMF locations
        args.push_back("-bios"); args.push_back("/usr/share/ovmf/OVMF.fd");
    }

    // Disk
    if (!vm.disk_path.empty()) {
        std::string drive = "file=" + vm.disk_path + ",if=virtio,format=";
        switch(vm.disk_format) {
            case DiskFormat::qcow2: drive += "qcow2"; break;
            case DiskFormat::raw:   drive += "raw"; break;
            case DiskFormat::vmdk:  drive += "vmdk"; break;
            case DiskFormat::vdi:   drive += "vdi"; break;
        }
        args.push_back("-drive"); args.push_back(drive);
    }

    // ISO
    if (!vm.iso_path.empty()) {
        args.push_back("-cdrom"); args.push_back(vm.iso_path);
    }

    // Boot order
    if (!vm.boot_order.empty()) {
        std::string boot = "order=" + vm.boot_order;
        if (vm.boot_menu) boot += ",menu=on";
        args.push_back("-boot"); args.push_back(boot);
    }

    // Display
    std::string disp;
    switch(vm.display) {
        case DisplayType::GTK:      disp = "gtk"; break;
        case DisplayType::SDL:      disp = "sdl"; break;
        case DisplayType::SPICE:    disp = "spice-app"; break;
        case DisplayType::VNC:      disp = "vnc=:0"; break;
        case DisplayType::EGL:      disp = "egl-headless"; break;
        case DisplayType::Headless: disp = "none"; break;
    }
    args.push_back("-display"); args.push_back(disp);

    // GPU
    std::string gpu_dev;
    switch(vm.gpu) {
        case GPUType::VGA:       gpu_dev = "VGA"; break;
        case GPUType::VirtIO:    gpu_dev = "virtio-gpu-pci"; break;
        case GPUType::QXL:       gpu_dev = "qxl-vga"; break;
        case GPUType::Cirrus:    gpu_dev = "cirrus-vga"; break;
        case GPUType::VMwareSVGA:gpu_dev = "vmware-svga"; break;
        case GPUType::None:      break;
    }
    if (!gpu_dev.empty()) {
        args.push_back("-device"); args.push_back(gpu_dev);
    }

    // Audio
    switch(vm.audio) {
        case AudioType::IntelHDA:
            args.push_back("-device"); args.push_back("intel-hda");
            args.push_back("-device"); args.push_back("hda-duplex");
            break;
        case AudioType::AC97:
            args.push_back("-device"); args.push_back("AC97");
            break;
        case AudioType::SB16:
            args.push_back("-device"); args.push_back("sb16");
            break;
        case AudioType::None: break;
    }

    // USB
    args.push_back("-usb");
    for (const auto& u : vm.usb_devices) {
        if (u.type == "tablet") {
            args.push_back("-device"); args.push_back("usb-tablet");
        } else if (u.type == "mouse") {
            args.push_back("-device"); args.push_back("usb-mouse");
        } else if (u.type == "keyboard") {
            args.push_back("-device"); args.push_back("usb-kbd");
        }
    }

    // Network
    switch(vm.net_mode) {
        case NetworkMode::User: {
            std::string netdev = "user,id=net0";
            for (const auto& pf : vm.port_forwards) {
                netdev += ",hostfwd=" + pf.protocol + "::" +
                    std::to_string(pf.host_port) + "-:" +
                    std::to_string(pf.guest_port);
            }
            args.push_back("-netdev"); args.push_back(netdev);
            args.push_back("-device"); args.push_back("virtio-net-pci,netdev=net0");
            break;
        }
        case NetworkMode::TAP:
            args.push_back("-netdev"); args.push_back("tap,id=net0,ifname=tap0,script=no");
            args.push_back("-device"); args.push_back("virtio-net-pci,netdev=net0");
            break;
        case NetworkMode::Bridge:
            args.push_back("-netdev"); args.push_back("bridge,id=net0,br=br0");
            args.push_back("-device"); args.push_back("virtio-net-pci,netdev=net0");
            break;
        case NetworkMode::Socket:
            args.push_back("-netdev"); args.push_back("socket,id=net0,listen=:4444");
            args.push_back("-device"); args.push_back("virtio-net-pci,netdev=net0");
            break;
    }

    // Shared Folders (VirtFS)
    for (size_t i = 0; i < vm.shared_folders.size(); i++) {
        const auto& sf = vm.shared_folders[i];
        std::string tag = "share" + std::to_string(i);
        std::string fsdev = "local,security_model=passthrough,id=" + tag +
            ",path=" + sf.host_path;
        if (sf.read_only) fsdev += ",readonly=on";
        args.push_back("-fsdev"); args.push_back(fsdev);
        args.push_back("-device"); args.push_back("virtio-9p-pci,fsdev=" + tag + ",mount_tag=" + tag);
    }

    // Serial console
    args.push_back("-serial"); args.push_back("stdio");

    // QEMU monitor
    args.push_back("-monitor"); args.push_back("unix:" + vm.vm_dir + "/monitor.sock,server,nowait");

    // Pidfile
    args.push_back("-pidfile"); args.push_back(vm.vm_dir + "/vm.pid");

    // Extra args (split by space, naive)
    if (!vm.extra_args.empty()) {
        std::istringstream iss(vm.extra_args);
        std::string token;
        while (iss >> token) args.push_back(token);
    }

    return args;
}

std::string CommandBuilder::buildCommand(const VMConfig& vm, const std::string& qemu_bin_dir) {
    std::string bin = archToQemuBin(vm.arch);
    if (!qemu_bin_dir.empty()) bin = qemu_bin_dir + "/" + bin;
    auto args = buildArgs(vm);
    std::ostringstream cmd;
    cmd << bin;
    for (const auto& a : args) cmd << " " << a;
    return cmd.str();
}

std::string CommandBuilder::formatCommand(const VMConfig& vm, const std::string& qemu_bin_dir) {
    std::string bin = archToQemuBin(vm.arch);
    if (!qemu_bin_dir.empty()) bin = qemu_bin_dir + "/" + bin;
    auto args = buildArgs(vm);
    std::ostringstream cmd;
    cmd << bin;
    for (size_t i = 0; i < args.size(); i++) {
        if (i % 2 == 0) cmd << " \\\n  " << args[i];
        else cmd << " " << args[i];
    }
    return cmd.str();
}

#include <algorithm>
#include "commandbuilder.h"
#include "vmconfig_io.h"
#include <sstream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>

// ============================================================
// Arch -> default binary name
// ============================================================
std::string CommandBuilder::archToQemuBin(VMArch arch, VMMode mode) {
    if (mode == VMMode::System) {
        const char* t[] = {
            "qemu-system-x86_64","qemu-system-i386",
            "qemu-system-aarch64","qemu-system-arm","qemu-system-arm",
            "qemu-system-riscv64","qemu-system-riscv32",
            "qemu-system-mips","qemu-system-mipsel",
            "qemu-system-mips64","qemu-system-mips64el",
            "qemu-system-mips","qemu-system-mipsel",   // mipsn32/n32el -> no sys binary
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
        int i = (int)arch;
        return (i>=0 && i<(int)(sizeof(t)/sizeof(t[0]))) ? t[i] : "qemu-system-x86_64";
    }
    const char* t[] = {
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
    int i = (int)arch;
    return (i>=0 && i<(int)(sizeof(t)/sizeof(t[0]))) ? t[i] : "qemu-x86_64";
}

// ============================================================
// Resolve the actual binary path for a VM
//   BinaryMode::Auto   -> /usr/bin/<arch-derived name>
//   BinaryMode::Custom -> /usr/bin/<custom_binary>
// ============================================================
std::string CommandBuilder::resolveBinary(const VMConfig& vm) {
    std::string name;
    if (vm.binary_mode == BinaryMode::Custom && !vm.custom_binary.empty())
        name = vm.custom_binary;
    else
        name = archToQemuBin(vm.arch, vm.mode);
    return "/usr/bin/" + name;
}

// ============================================================
// Scan /usr/bin/ for installed qemu-system-* binaries
// ============================================================
std::vector<std::string> CommandBuilder::listInstalledBinaries(VMMode mode) {
    std::vector<std::string> out;
    const char* prefix = (mode == VMMode::System) ? "qemu-system-" : "qemu-";
    DIR* d = opendir("/usr/bin");
    if (!d) return out;
    struct dirent* e;
    while ((e = readdir(d))) {
        std::string n = e->d_name;
        if (n.rfind(prefix, 0) == 0) {
            // For user-mode exclude "qemu-system-*"
            if (mode == VMMode::UserMode && n.rfind("qemu-system-",0)==0) continue;
            out.push_back(n);
        }
    }
    closedir(d);
    std::sort(out.begin(), out.end());
    return out;
}

// ============================================================
// Validate custom binary name:
//   - no spaces, no slashes, no flag chars after name
//   - must start with qemu-
//   - must exist in /usr/bin/
// Returns "" on OK, error message on fail
// ============================================================
std::string CommandBuilder::validateCustomBinary(const std::string& name) {
    if (name.empty())         return "Binary name is empty.";
    if (name.find(' ')  != std::string::npos) return "Binary name must not contain spaces.";
    if (name.find('/')  != std::string::npos) return "Binary name must not contain slashes (always resolved in /usr/bin/).";
    if (name.find('-', name.find('-')+1) != std::string::npos) {
        // allow qemu-system-foo but not qemu-system-foo -flag
        // real check: must be alnum + hyphen only
    }
    // Allow only alphanum + hyphen + underscore
    for (char c : name)
        if (!isalnum(c) && c!='-' && c!='_')
            return std::string("Invalid character '") + c + "' in binary name.";
    if (name.rfind("qemu-",0) != 0)
        return "Binary must start with 'qemu-'.";
    struct stat st{};
    if (stat(("/usr/bin/"+name).c_str(), &st) != 0)
        return "Binary '/usr/bin/" + name + "' not found.";
    return "";
}

// ============================================================
// Build argv
// ============================================================
std::vector<std::string> CommandBuilder::buildArgs(const VMConfig& vm) {
    std::vector<std::string> args;

    if (vm.mode == VMMode::UserMode) {
        if (!vm.extra_args.empty()) {
            std::istringstream iss(vm.extra_args);
            std::string t; while (iss>>t) args.push_back(t);
        }
        return args;
    }

    // Machine
    const char* mach_map[] = {"pc","q35","virt","microvm",""};
    std::string mach = (vm.machine==MachineType::custom)
        ? vm.machine_custom : mach_map[(int)vm.machine];
    args.push_back("-machine"); args.push_back(mach);

    // Accelerator
    switch (vm.accel) {
        case Accelerator::TCG:
            args.push_back("-accel"); args.push_back("tcg,thread=multi"); break;
        case Accelerator::KVM:
            args.push_back("-accel"); args.push_back("kvm"); break;
        case Accelerator::KVM_LBT:
            args.push_back("-accel"); args.push_back("kvm");
            // LBT extension exposed via cpu flag
            args.push_back("-cpu");   args.push_back(vm.cpu_model + ",lbt=on");
            goto skip_cpu; // cpu already added
    }
    // CPU + SMP (normal path)
    args.push_back("-cpu"); args.push_back(vm.cpu_model);
    skip_cpu:
    args.push_back("-smp");
    args.push_back(std::to_string(vm.sockets*vm.cores*vm.threads)
        +",sockets="+std::to_string(vm.sockets)
        +",cores="  +std::to_string(vm.cores)
        +",threads="+std::to_string(vm.threads));

    // RAM
    args.push_back("-m"); args.push_back(std::to_string(vm.ram_mb));
    if (vm.ballooning) { args.push_back("-device"); args.push_back("virtio-balloon-pci"); }

    // UEFI
    if (vm.uefi) { args.push_back("-bios"); args.push_back("/usr/share/ovmf/OVMF.fd"); }

    // Disk
    if (!vm.disk_path.empty()) {
        const char* fmts[]={"qcow2","raw","vmdk","vdi"};
        args.push_back("-drive");
        args.push_back("file="+vm.disk_path+",if=virtio,format="+fmts[(int)vm.disk_format]);
    }

    // ISO
    if (!vm.iso_path.empty()) { args.push_back("-cdrom"); args.push_back(vm.iso_path); }

    // Boot
    if (!vm.boot_order.empty()) {
        std::string b = "order="+vm.boot_order;
        if (vm.boot_menu) b+=",menu=on";
        args.push_back("-boot"); args.push_back(b);
    }

    // Display — SDL default
    const char* disp_map[]={"gtk","sdl","spice-app","vnc=:0","egl-headless","none"};
    args.push_back("-display"); args.push_back(disp_map[(int)vm.display]);

    // GPU
    const char* gpu_map[]={"VGA","virtio-gpu-pci","qxl-vga","cirrus-vga","vmware-svga",""};
    if (vm.gpu!=GPUType::None) {
        args.push_back("-device"); args.push_back(gpu_map[(int)vm.gpu]);
    }

    // Audio
    switch (vm.audio) {
        case AudioType::IntelHDA:
            args.push_back("-device"); args.push_back("intel-hda");
            args.push_back("-device"); args.push_back("hda-duplex"); break;
        case AudioType::AC97:
            args.push_back("-device"); args.push_back("AC97"); break;
        case AudioType::SB16:
            args.push_back("-device"); args.push_back("sb16"); break;
        default: break;
    }

    // USB
    args.push_back("-usb");
    for (const auto& u : vm.usb_devices) {
        if      (u.type=="tablet")   {args.push_back("-device");args.push_back("usb-tablet");}
        else if (u.type=="mouse")    {args.push_back("-device");args.push_back("usb-mouse");}
        else if (u.type=="keyboard") {args.push_back("-device");args.push_back("usb-kbd");}
    }

    // Network
    switch (vm.net_mode) {
        case NetworkMode::User: {
            std::string nd="user,id=net0";
            for (const auto& pf:vm.port_forwards)
                nd+=",hostfwd="+pf.protocol+"::"+std::to_string(pf.host_port)+"-:"+std::to_string(pf.guest_port);
            args.push_back("-netdev"); args.push_back(nd);
            args.push_back("-device"); args.push_back("virtio-net-pci,netdev=net0"); break;
        }
        case NetworkMode::TAP:
            args.push_back("-netdev"); args.push_back("tap,id=net0,ifname=tap0,script=no");
            args.push_back("-device"); args.push_back("virtio-net-pci,netdev=net0"); break;
        case NetworkMode::Bridge:
            args.push_back("-netdev"); args.push_back("bridge,id=net0,br=br0");
            args.push_back("-device"); args.push_back("virtio-net-pci,netdev=net0"); break;
        case NetworkMode::Socket:
            args.push_back("-netdev"); args.push_back("socket,id=net0,listen=:4444");
            args.push_back("-device"); args.push_back("virtio-net-pci,netdev=net0"); break;
    }

    // Shared folders
    for (size_t i=0;i<vm.shared_folders.size();i++) {
        const auto& sf=vm.shared_folders[i];
        std::string tag="share"+std::to_string(i);
        std::string fsdev="local,security_model=passthrough,id="+tag+",path="+sf.host_path;
        if (sf.read_only) fsdev+=",readonly=on";
        args.push_back("-fsdev"); args.push_back(fsdev);
        args.push_back("-device"); args.push_back("virtio-9p-pci,fsdev="+tag+",mount_tag="+tag);
    }

    // Monitor socket + pidfile
    args.push_back("-monitor"); args.push_back("unix:"+vm.vm_dir+"/monitor.sock,server,nowait");
    args.push_back("-pidfile");  args.push_back(vm.vm_dir+"/vm.pid");

    // Extra args LAST
    if (!vm.extra_args.empty()) {
        std::istringstream iss(vm.extra_args);
        std::string t; while (iss>>t) args.push_back(t);
    }

    return args;
}

std::string CommandBuilder::buildCommand(const VMConfig& vm, const std::string&) {
    std::string bin = resolveBinary(vm);
    auto args = buildArgs(vm);
    std::ostringstream o; o << bin;
    for (const auto& a:args) o<<" "<<a;
    return o.str();
}

std::string CommandBuilder::formatCommand(const VMConfig& vm, const std::string&) {
    std::string bin = resolveBinary(vm);
    auto args = buildArgs(vm);
    std::ostringstream o; o << bin;
    for (size_t i=0;i<args.size();i++) {
        if (i%2==0) o<<" \\\n  "<<args[i];
        else        o<<" "<<args[i];
    }
    return o.str();
}

// ============================================================
// launchVM — pure in-memory, zero tmp files
// ============================================================
pid_t CommandBuilder::launchVM(const std::string& vm_dir, const std::string&) {
    VMConfig vm;
    if (!VMConfigIO::load(vm_dir, vm)) return -1;

    // Validate custom binary if set
    if (vm.binary_mode == BinaryMode::Custom) {
        std::string err = validateCustomBinary(vm.custom_binary);
        if (!err.empty()) return -1;
    }

    if (vm.save_script_to_vm_folder) writeStartScript(vm);

    std::string bin = resolveBinary(vm);
    auto args = buildArgs(vm);

    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(bin.c_str()));
    for (auto& a:args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid==0) { execvp(argv[0],argv.data()); _exit(127); }
    return pid;
}

bool CommandBuilder::writeStartScript(const VMConfig& vm, const std::string&) {
    if (vm.vm_dir.empty()) return false;
    std::string path = vm.vm_dir+"/start.sh";
    std::ofstream f(path);
    if (!f) return false;
    f << "#!/bin/bash\n# Generated by qemu-manager\n# VM: "<<vm.name<<"\n\n"
      << formatCommand(vm) << "\n";
    f.close();
    ::chmod(path.c_str(),0755);
    return true;
}

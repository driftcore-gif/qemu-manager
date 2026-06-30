#include "commandbuilder.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// ── arch → binary name ────────────────────────────────────────────────
std::string CommandBuilder::archToQemuBin(VMArch arch, VMMode mode) {
    if (mode == VMMode::System) {
        static const char* t[] = {
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
        int i=(int)arch; return(i>=0&&i<(int)(sizeof(t)/sizeof(t[0])))?t[i]:"qemu-system-x86_64";
    }
    static const char* u[] = {
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
    int i=(int)arch; return(i>=0&&i<(int)(sizeof(u)/sizeof(u[0])))?u[i]:"qemu-x86_64";
}

std::string CommandBuilder::resolveBinary(const VMConfig& vm) {
    std::string name;
    if (vm.binary_mode==BinaryMode::Custom && !vm.custom_binary.empty())
        name=vm.custom_binary;
    else
        name=archToQemuBin(vm.arch, vm.mode);
    // Check Termux path first, then standard
    for (const char* dir : {"/data/data/com.termux/files/usr/bin", "/usr/bin"}) {
        std::string full=std::string(dir)+"/"+name;
        if (access(full.c_str(),X_OK)==0) return full;
    }
    return "/usr/bin/"+name;
}

std::vector<std::string> CommandBuilder::listInstalledBinaries(VMMode mode) {
    std::vector<std::string> out;
    const char* prefix=(mode==VMMode::System)?"qemu-system-":"qemu-";
    for (const char* dir : {"/usr/bin","/data/data/com.termux/files/usr/bin"}) {
        DIR* d=opendir(dir); if(!d) continue;
        struct dirent* e;
        while((e=readdir(d))) {
            std::string n=e->d_name;
            if(n.rfind(prefix,0)==0) {
                if(mode==VMMode::UserMode && n.rfind("qemu-system-",0)==0) continue;
                if(std::find(out.begin(),out.end(),n)==out.end()) out.push_back(n);
            }
        }
        closedir(d);
    }
    std::sort(out.begin(),out.end());
    return out;
}

std::string CommandBuilder::validateCustomBinary(const std::string& name) {
    if(name.empty()) return "Binary name is empty.";
    if(name.find(' ')!=std::string::npos) return "Must not contain spaces.";
    if(name.find('/')!=std::string::npos) return "Must not contain slashes.";
    if(name.rfind("qemu-",0)!=0) return "Must start with 'qemu-'.";
    return "";
}

// ── Build argument list — full QEMU 11 feature set ────────────────────
std::vector<std::string> CommandBuilder::buildArgs(const VMConfig& vm) {
    std::vector<std::string> a;
    auto add=[&](const std::string& s){ a.push_back(s); };
    auto flag=[&](const std::string& f,const std::string& v){ add(f); add(v); };

    if(vm.mode==VMMode::UserMode) {
        if(!vm.extra_args.empty()) {
            std::istringstream ss(vm.extra_args);
            std::string tok; while(ss>>tok) add(tok);
        }
        return a;
    }

    // ── Machine ───────────────────────────────────────────────────────
    std::string mach;
    switch(vm.machine) {
        case MachineType::pc:             mach="pc"; break;
        case MachineType::virt:           mach="virt"; break;
        case MachineType::microvm:        mach="microvm"; break;
        case MachineType::sbsa_ref:       mach="sbsa-ref"; break;
        case MachineType::virt_acpi:      mach="virt,acpi=on"; break;
        case MachineType::x86_64_microvm: mach="microvm,x-option-roms=off,isa-serial=off,pit=off,pic=off"; break;
        case MachineType::custom:         mach=vm.machine_custom.empty()?"q35":vm.machine_custom; break;
        default:                          mach="q35"; break;
    }
    flag("-machine", mach);

    // ── Accelerator ───────────────────────────────────────────────────
    static const int TB_MB[]={64,128,256,512,1024};
    int tb=TB_MB[(int)vm.tb_size];
    switch(vm.accel) {
        case Accelerator::TCG: {
            std::string opts="tcg,thread=";
            opts+=(vm.tcg_mttcg?"multi":"single");
            opts+=",tb-size="+std::to_string(tb);
            flag("-accel",opts); break;
        }
        case Accelerator::KVM:     flag("-accel","kvm"); break;
        case Accelerator::KVM_LBT: flag("-accel","kvm"); break;
        case Accelerator::WHPX:    flag("-accel","whpx"); break;
        case Accelerator::HVF:     flag("-accel","hvf"); break;
        case Accelerator::NVMM:    flag("-accel","nvmm"); break;
        case Accelerator::Xen:     flag("-accel","xen"); break;
    }

    // ── CPU ───────────────────────────────────────────────────────────
    {
        std::string cpu=vm.cpu_model;
        if(vm.accel==Accelerator::KVM_LBT) cpu+=",lbt=on";
        if(vm.accel==Accelerator::KVM && vm.kvm_nested) cpu+=",vmx=on";
        if(!vm.cpu_flags.empty()) cpu+=","+vm.cpu_flags;
        if(!vm.cpu_migratable) cpu+=",migratable=off";
        flag("-cpu",cpu);
    }
    flag("-smp", std::to_string(vm.sockets*vm.cores*vm.threads)
        +",sockets="+std::to_string(vm.sockets)
        +",cores="+std::to_string(vm.cores)
        +",threads="+std::to_string(vm.threads));

    // ── Memory ────────────────────────────────────────────────────────
    if(vm.memfd_backend) {
        flag("-object","memory-backend-memfd,id=mem,size="+std::to_string(vm.ram_mb)+"M,share=on");
        flag("-numa","node,memdev=mem");
    } else {
        flag("-m",std::to_string(vm.ram_mb));
    }
    if(vm.mem_slots>0)
        flag("-m", std::to_string(vm.ram_mb)+"M,slots="+std::to_string(vm.mem_slots)
            +",maxmem="+std::to_string(vm.ram_mb*2)+"M");
    if(vm.ballooning){ add("-device"); add("virtio-balloon-pci"); }

    // ── Firmware ──────────────────────────────────────────────────────
    switch(vm.firmware) {
        case Firmware::OVMF:
            flag("-drive","if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE.fd"); break;
        case Firmware::OVMF_SecureBoot:
            flag("-drive","if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE.secboot.fd");
            if(vm.secure_boot){ add("-global"); add("driver=cfi.pflash01,property=secure,value=on"); }
            break;
        case Firmware::U_Boot:
            flag("-bios","/usr/lib/u-boot/qemu-arm/u-boot.bin"); break;
        case Firmware::EDK2_ARM:
            flag("-drive","if=pflash,format=raw,readonly=on,file=/usr/share/qemu-efi-aarch64/QEMU_EFI.fd"); break;
        default: break; // SeaBIOS = default
    }
    {
        std::string b="order="+vm.boot_order;
        if(vm.boot_menu) b+=",menu=on";
        flag("-boot",b);
    }

    // ── Storage ───────────────────────────────────────────────────────
    static const char* FMTS[]={"qcow2","raw","vmdk","vdi"};
    std::string fmt=FMTS[(int)vm.disk_format];
    if(!vm.disk_path.empty()) {
        std::string discard=vm.disk_discard?",discard=unmap,detect-zeroes=unmap":"";
        if(vm.disk_interface==DiskInterface::nvme) {
            flag("-drive","file="+vm.disk_path+",id=drive0,format="+fmt+",if=none"+discard);
            flag("-device","nvme,drive=drive0,serial=qemu-nvme0,num-queues="
                +std::to_string(vm.cores*vm.threads));
        } else if(vm.use_scsi_ctrl && vm.disk_interface==DiskInterface::scsi) {
            flag("-device","virtio-scsi-pci,id=scsi0");
            flag("-drive","file="+vm.disk_path+",id=drive0,format="+fmt+",if=none"+discard);
            flag("-device","scsi-hd,bus=scsi0.0,drive=drive0");
        } else {
            static const char* IFACES[]={"virtio","scsi","none","ide","sata"};
            std::string iface=IFACES[(int)vm.disk_interface];
            flag("-drive","file="+vm.disk_path+",if="+iface+",format="+fmt+discard);
        }
    }
    if(!vm.disk2_path.empty())
        flag("-drive","file="+vm.disk2_path+",if=virtio,format="+fmt);
    if(!vm.iso_path.empty())
        flag("-cdrom",vm.iso_path);

    // ── Display ───────────────────────────────────────────────────────
    static const char* DISP[]={"gtk","sdl","spice-app","vnc=:0","egl-headless","dbus","none"};
    std::string disp_val=DISP[(int)vm.display];
    // For VirGL add gl=on to display
    if(vm.virgl_enabled &&
       (vm.gpu==GPUType::VirtIO_GPU_GL) &&
       (vm.display==DisplayType::GTK||vm.display==DisplayType::SDL||vm.display==DisplayType::EGL))
        disp_val+=",gl=on";
    flag("-display",disp_val);

    // Resolution
    if(!vm.resolution.empty() && vm.resolution!="default") {
        std::string r=vm.resolution;
        auto pos=r.find('x');
        if(pos!=std::string::npos) { r[pos]=','; flag("-g",r); }
    }

    // ── GPU ───────────────────────────────────────────────────────────
    switch(vm.gpu) {
        case GPUType::VGA:               flag("-device","VGA"); break;
        case GPUType::VirtIO_GPU:        flag("-device","virtio-gpu-pci"); break;
        case GPUType::VirtIO_GPU_GL:     flag("-device","virtio-gpu-gl-pci"); break;
        case GPUType::VirtIO_GPU_Rutabaga:
            flag("-device","virtio-gpu-rutabaga,cross-domain=on,wsi=headless"); break;
        case GPUType::QXL:               flag("-device","qxl-vga"); break;
        case GPUType::Cirrus:            flag("-device","cirrus-vga"); break;
        case GPUType::VMwareSVGA:        flag("-device","vmware-svga"); break;
        case GPUType::ramfb:             flag("-device","ramfb"); break;
        default: break;
    }

    // ── Audio ─────────────────────────────────────────────────────────
    switch(vm.audio) {
        case AudioType::IntelHDA:
            flag("-device","intel-hda"); flag("-device","hda-duplex"); break;
        case AudioType::AC97:        flag("-device","AC97"); break;
        case AudioType::SB16:        flag("-device","sb16"); break;
        case AudioType::VirtIO_Sound:flag("-device","virtio-sound-pci"); break;
        default: break;
    }

    // ── USB ───────────────────────────────────────────────────────────
    if(vm.usb_version==UsbVersion::USB3_XHCI)
        flag("-device","qemu-xhci,id=xhci");
    else
        add("-usb");
    if(vm.usb_tablet){ add("-device"); add("usb-tablet"); }

    // ── Network ───────────────────────────────────────────────────────
    switch(vm.net_mode) {
        case NetworkMode::User: {
            std::string nd="user,id=net0";
            if(!vm.net_dns.empty())  nd+=",dns="+vm.net_dns;
            if(!vm.smb_share.empty())nd+=",smb="+vm.smb_share;
            flag("-netdev",nd);
            flag("-device","virtio-net-pci,netdev=net0"); break;
        }
        case NetworkMode::TAP:
            flag("-netdev","tap,id=net0,ifname=tap0,script=no,downscript=no");
            flag("-device","virtio-net-pci,netdev=net0"); break;
        case NetworkMode::Bridge:
            flag("-netdev","bridge,id=net0,br=br0");
            flag("-device","virtio-net-pci,netdev=net0"); break;
        case NetworkMode::Socket:
            flag("-netdev","socket,id=net0,listen=:4444");
            flag("-device","virtio-net-pci,netdev=net0"); break;
        case NetworkMode::VDE:
            flag("-netdev","vde,id=net0");
            flag("-device","virtio-net-pci,netdev=net0"); break;
        case NetworkMode::VirtIO_VHostNet:
            flag("-netdev","vhost-user,id=net0,chardev=chr0,vhostforce=on");
            flag("-device","virtio-net-pci,netdev=net0,mrg_rxbuf=on"); break;
    }

    // ── IOMMU ─────────────────────────────────────────────────────────
    switch(vm.iommu) {
        case IommuType::Intel:       flag("-device","intel-iommu,intremap=on,caching-mode=on"); break;
        case IommuType::SMMUv3:      flag("-device","arm-smmuv3"); break;
        case IommuType::VirtIO_IOMMU:flag("-device","virtio-iommu-pci"); break;
        default: break;
    }

    // ── TPM ───────────────────────────────────────────────────────────
    switch(vm.tpm) {
        case TpmType::TIS:
            flag("-tpmdev","passthrough,id=tpm0,path=/dev/tpm0");
            flag("-device","tpm-tis,tpmdev=tpm0"); break;
        case TpmType::CRB:
            flag("-tpmdev","emulator,id=tpm0,chardev=chrtpm");
            flag("-chardev","socket,id=chrtpm,path=/var/run/swtpm/sock");
            flag("-device","tpm-crb,tpmdev=tpm0"); break;
        default: break;
    }

    // ── VirtIO RNG ────────────────────────────────────────────────────
    if(vm.virtio_rng){ add("-device"); add("virtio-rng-pci"); }

    // ── QEMU 11 misc ──────────────────────────────────────────────────
    if(vm.snapshot_mode) add("-snapshot");
    if(vm.no_reboot)     add("-no-reboot");

    // ── Monitor ───────────────────────────────────────────────────────
    if(!vm.vm_dir.empty()) {
        flag("-monitor","unix:"+vm.vm_dir+"/monitor.sock,server,nowait");
        flag("-pidfile", vm.vm_dir+"/vm.pid");
    }

    // ── Extra args LAST ───────────────────────────────────────────────
    if(!vm.extra_args.empty()) {
        std::istringstream ss(vm.extra_args);
        std::string tok; while(ss>>tok) add(tok);
    }
    return a;
}

// ── Format helpers ────────────────────────────────────────────────────
std::string CommandBuilder::buildCommand(const VMConfig& vm) {
    std::string bin=resolveBinary(vm);
    std::ostringstream s; s<<bin;
    for(auto& a:buildArgs(vm)){
        s<<' ';
        if(a.find(' ')!=std::string::npos) s<<'"'<<a<<'"'; else s<<a;
    }
    return s.str();
}

std::string CommandBuilder::formatCommand(const VMConfig& vm) {
    std::string bin=resolveBinary(vm);
    std::ostringstream s; s<<bin;
    auto args=buildArgs(vm);
    for(size_t i=0;i<args.size();) {
        s<<" \\\n  "<<args[i];
        if(args[i].rfind("-",0)==0 && i+1<args.size() && args[i+1].rfind("-",0)!=0) {
            ++i;
            if(args[i].find(' ')!=std::string::npos) s<<" \""<<args[i]<<"\"";
            else s<<" "<<args[i];
        }
        ++i;
    }
    return s.str();
}

void CommandBuilder::writeStartScript(const VMConfig& vm) {
    if(vm.vm_dir.empty()) return;
    std::ofstream f(vm.vm_dir+"/start.sh");
    if(!f) return;
    f<<"#!/bin/bash\n# QEMU Manager — generated start script\n# VM: "<<vm.name<<"\n\n";
    f<<formatCommand(vm)<<"\n";
    f.close();
    chmod((vm.vm_dir+"/start.sh").c_str(),0755);
}

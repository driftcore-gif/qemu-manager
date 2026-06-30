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
        int i=(int)arch;
        return (i>=0&&i<(int)(sizeof(t)/sizeof(t[0])))?t[i]:"qemu-system-x86_64";
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
    int i=(int)arch;
    return (i>=0&&i<(int)(sizeof(u)/sizeof(u[0])))?u[i]:"qemu-x86_64";
}

std::string CommandBuilder::resolveBinary(const VMConfig& vm) {
    std::string name;
    if (vm.binary_mode==BinaryMode::Custom && !vm.custom_binary.empty())
        name = vm.custom_binary;
    else
        name = archToQemuBin(vm.arch, vm.mode);
    for (const char* dir : {"/data/data/com.termux/files/usr/bin", "/usr/bin"}) {
        std::string full = std::string(dir)+"/"+name;
        if (access(full.c_str(), X_OK)==0) return full;
    }
    return "/usr/bin/"+name;
}

std::vector<std::string> CommandBuilder::listInstalledBinaries(VMMode mode) {
    std::vector<std::string> out;
    const char* prefix = (mode==VMMode::System)?"qemu-system-":"qemu-";
    for (const char* dir : {"/usr/bin","/data/data/com.termux/files/usr/bin"}) {
        DIR* d = opendir(dir); if(!d) continue;
        struct dirent* e;
        while((e=readdir(d))) {
            std::string n = e->d_name;
            if(n.rfind(prefix,0)==0) {
                if(mode==VMMode::UserMode && n.rfind("qemu-system-",0)==0) continue;
                if(std::find(out.begin(),out.end(),n)==out.end()) out.push_back(n);
            }
        }
        closedir(d);
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::string CommandBuilder::validateCustomBinary(const std::string& name) {
    if(name.empty())                              return "Binary name is empty.";
    if(name.find(' ')!=std::string::npos)         return "Must not contain spaces.";
    if(name.find('/')!=std::string::npos)         return "Must not contain slashes.";
    if(name.rfind("qemu-",0)!=0)                  return "Must start with 'qemu-'.";
    return "";
}

// ── Full QEMU 11 command builder ──────────────────────────────────────
std::vector<std::string> CommandBuilder::buildArgs(const VMConfig& vm) {
    std::vector<std::string> a;
    auto add  = [&](const std::string& s){ a.push_back(s); };
    auto flag = [&](const std::string& f, const std::string& v){ add(f); add(v); };

    if(vm.mode==VMMode::UserMode) {
        if(!vm.extra_args.empty()) {
            std::istringstream ss(vm.extra_args);
            std::string tok; while(ss>>tok) add(tok);
        }
        return a;
    }

    // ── Machine ───────────────────────────────────────────────────────
    {
        std::string mach;
        switch(vm.machine) {
            case MachineType::pc:             mach="pc"; break;
            case MachineType::virt:           mach="virt"; break;
            case MachineType::microvm:        mach="microvm"; break;
            case MachineType::sbsa_ref:       mach="sbsa-ref"; break;
            case MachineType::virt_acpi:      mach="virt,acpi=on"; break;
            case MachineType::x86_64_microvm: mach="microvm,x-option-roms=off,isa-serial=off,pit=off,pic=off"; break;
            // QEMU 11: Nitro Enclave machine type
            case MachineType::nitro_enclave:  mach="nitro-enclave"; break;
            case MachineType::custom:         mach=vm.machine_custom.empty()?"q35":vm.machine_custom; break;
            default:                          mach="q35"; break;
        }
        flag("-machine", mach);
    }

    // ── Accelerator ───────────────────────────────────────────────────
    {
        static const int TB_MB[]={64,128,256,512,1024};
        int tb = TB_MB[(int)vm.tb_size];
        switch(vm.accel) {
            case Accelerator::TCG: {
                std::string opts = "tcg,thread=";
                opts += (vm.tcg_mttcg?"multi":"single");
                opts += ",tb-size="+std::to_string(tb);
                flag("-accel", opts); break;
            }
            case Accelerator::KVM:     flag("-accel","kvm"); break;
            case Accelerator::KVM_LBT: flag("-accel","kvm"); break;  // +lbt via cpu flag
            case Accelerator::WHPX:    flag("-accel","whpx"); break;
            case Accelerator::HVF:     flag("-accel","hvf"); break;
            case Accelerator::NVMM:    flag("-accel","nvmm"); break;
            case Accelerator::Xen:     flag("-accel","xen"); break;
            // QEMU 11: Nitro + MSHV
            case Accelerator::Nitro:   flag("-accel","nitro"); break;
            case Accelerator::MSHV:    flag("-accel","mshv"); break;
        }
    }

    // ── CPU ───────────────────────────────────────────────────────────
    {
        // QEMU 10: named CPU model shortcuts for new Intel silicon
        std::string cpu = vm.cpu_model;
        if (!vm.x86_cpu_gen.empty()) cpu = vm.x86_cpu_gen; // e.g. DiamondRapids
        if(vm.accel==Accelerator::KVM_LBT)          cpu += ",lbt=on";
        if(vm.accel==Accelerator::KVM && vm.kvm_nested) cpu += ",vmx=on";
        // QEMU 11: CET virtualisation
        if(vm.kvm_cet && (vm.accel==Accelerator::KVM||vm.accel==Accelerator::KVM_LBT))
            cpu += ",cet=on";
        if(!vm.cpu_flags.empty())                   cpu += ","+vm.cpu_flags;
        if(!vm.cpu_migratable)                      cpu += ",migratable=off";
        flag("-cpu", cpu);
    }
    flag("-smp", std::to_string(vm.sockets*vm.cores*vm.threads)
        +",sockets="+std::to_string(vm.sockets)
        +",cores="+std::to_string(vm.cores)
        +",threads="+std::to_string(vm.threads));

    // ── Memory ────────────────────────────────────────────────────────
    if(vm.memfd_backend) {
        flag("-object","memory-backend-memfd,id=mem,size="+std::to_string(vm.ram_mb)+"M,share=on");
        flag("-numa","node,memdev=mem");
    } else if(vm.mem_slots>0) {
        flag("-m", std::to_string(vm.ram_mb)+"M,slots="+std::to_string(vm.mem_slots)
            +",maxmem="+std::to_string(vm.ram_mb*2)+"M");
    } else {
        flag("-m", std::to_string(vm.ram_mb));
    }
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
        default: break;
    }
    { std::string b="order="+vm.boot_order; if(vm.boot_menu) b+=",menu=on"; flag("-boot",b); }

    // ── Storage ───────────────────────────────────────────────────────
    static const char* FMTS[]={"qcow2","raw","vmdk","vdi"};
    std::string fmt = FMTS[(int)vm.disk_format];
    if(!vm.disk_path.empty()) {
        std::string discard = vm.disk_discard?",discard=unmap,detect-zeroes=unmap":"";
        if(vm.disk_interface==DiskInterface::nvme) {
            flag("-drive","file="+vm.disk_path+",id=drive0,format="+fmt+",if=none"+discard);
            flag("-device","nvme,drive=drive0,serial=qemu-nvme0,num-queues="
                +std::to_string(vm.cores*vm.threads));
        } else if(vm.use_scsi_ctrl && vm.disk_interface==DiskInterface::scsi) {
            // QEMU 10: virtio-scsi multiqueue — one iothread per vcpu
            std::string scsi_dev = "virtio-scsi-pci,id=scsi0";
            if(vm.scsi_multiqueue)
                scsi_dev += ",num-queues="+std::to_string(vm.cores*vm.threads);
            flag("-device", scsi_dev);
            flag("-drive","file="+vm.disk_path+",id=drive0,format="+fmt+",if=none"+discard);
            flag("-device","scsi-hd,bus=scsi0.0,drive=drive0");
        } else {
            static const char* IFACES[]={"virtio","scsi","none","ide","sata"};
            flag("-drive","file="+vm.disk_path+",if="+std::string(IFACES[(int)vm.disk_interface])
                +",format="+fmt+discard);
        }
    }
    if(!vm.disk2_path.empty())
        flag("-drive","file="+vm.disk2_path+",if=virtio,format="+fmt);
    if(!vm.iso_path.empty())
        flag("-cdrom", vm.iso_path);

    // ── Display ───────────────────────────────────────────────────────
    {
        static const char* DISP[]={"gtk","sdl","spice-app","vnc=:0","egl-headless","dbus","none"};
        std::string disp_val = DISP[(int)vm.display];
        if(vm.virgl_enabled && vm.gpu==GPUType::VirtIO_GPU_GL &&
           (vm.display==DisplayType::GTK||vm.display==DisplayType::SDL||vm.display==DisplayType::EGL))
            disp_val += ",gl=on";
        flag("-display", disp_val);
        if(!vm.resolution.empty() && vm.resolution!="default") {
            std::string r=vm.resolution; auto pos=r.find('x');
            if(pos!=std::string::npos) { r[pos]=','; flag("-g",r); }
        }
    }

    // ── GPU ───────────────────────────────────────────────────────────
    {
        int head = 0;
        // Parse extra outputs for multi-head (QEMU 11)
        std::vector<std::string> extra_res;
        if(!vm.gpu_extra_outputs.empty()) {
            std::istringstream ss(vm.gpu_extra_outputs);
            std::string tok; while(std::getline(ss,tok,',')) { extra_res.push_back(tok); }
        }
        auto gpu_head = [&](const std::string& devname, int idx) {
            std::string dev = devname;
            if(idx>0) dev += ",id=gpu"+std::to_string(idx);
            if(!extra_res.empty() && idx>0 && idx<=(int)extra_res.size()) {
                auto& r = extra_res[idx-1];
                auto p = r.find('x');
                if(p!=std::string::npos)
                    dev += ",xres="+r.substr(0,p)+",yres="+r.substr(p+1);
            }
            add("-device"); add(dev);
        };
        switch(vm.gpu) {
            case GPUType::VirtIO_GPU:
                gpu_head("virtio-gpu-pci", head++); break;
            case GPUType::VirtIO_GPU_GL:
                gpu_head("virtio-gpu-gl-pci", head++);
                for(int i=0;i<(int)extra_res.size();i++) gpu_head("virtio-gpu-gl-pci",head++);
                break;
            case GPUType::VirtIO_GPU_Rutabaga:
                gpu_head("virtio-gpu-rutabaga", head++); break;
            // QEMU 11: native context driver (vhost-user-gpu)
            case GPUType::VirtIO_GPU_NativeCtx:
                flag("-chardev","socket,id=vgpu,path=/tmp/vgpu.sock");
                add("-device"); add("vhost-user-gpu,chardev=vgpu"); break;
            case GPUType::QXL:
                add("-device"); add("qxl-vga"); break;
            case GPUType::Cirrus:
                add("-device"); add("cirrus-vga"); break;
            case GPUType::VMwareSVGA:
                add("-device"); add("vmware-svga"); break;
            case GPUType::ramfb:
                add("-device"); add("ramfb"); break;
            case GPUType::None:
                add("-nographic"); break;
            default: add("-device"); add("virtio-gpu-pci"); break;
        }
    }

    // ── Audio ─────────────────────────────────────────────────────────
    switch(vm.audio) {
        case AudioType::IntelHDA:   add("-device"); add("intel-hda"); add("-device"); add("hda-duplex"); break;
        case AudioType::AC97:       add("-device"); add("AC97"); break;
        case AudioType::SB16:       add("-device"); add("sb16"); break;
        case AudioType::VirtIO_Sound: add("-device"); add("virtio-sound-pci"); break;
        case AudioType::None: break;
    }

    // ── USB ───────────────────────────────────────────────────────────
    if(vm.usb_version==UsbVersion::USB3_XHCI) {
        add("-device"); add("qemu-xhci");
    } else {
        add("-device"); add("usb-ehci");
    }
    if(vm.usb_tablet){ add("-device"); add("usb-tablet"); }

    // ── Network ───────────────────────────────────────────────────────
    {
        std::string net;
        switch(vm.net_mode) {
            case NetworkMode::User: {
                net = "user,id=net0";
                if(!vm.net_dns.empty())  net += ",dns="+vm.net_dns;
                if(!vm.smb_share.empty()) net += ",smb="+vm.smb_share;
                for(auto& pf : vm.port_forwards)
                    net += ",hostfwd="+pf.protocol+":0.0.0.0:"+std::to_string(pf.host_port)
                           +"::"+std::to_string(pf.guest_port);
                flag("-netdev", net);
                flag("-device","virtio-net-pci,netdev=net0");
                break;
            }
            case NetworkMode::TAP:
                flag("-netdev","tap,id=net0,ifname=tap0,script=no,downscript=no");
                flag("-device","virtio-net-pci,netdev=net0"); break;
            case NetworkMode::Bridge:
                flag("-netdev","bridge,id=net0,br=virbr0");
                flag("-device","virtio-net-pci,netdev=net0"); break;
            case NetworkMode::VirtIO_VHostNet:
                // QEMU 11: vhost-user network (high-performance)
                flag("-chardev","socket,id=chr_vhost,path=/tmp/vhost-user.sock");
                flag("-netdev","vhost-user,id=net0,chardev=chr_vhost,queues="+std::to_string(vm.cores));
                flag("-device","virtio-net-pci,netdev=net0,mq=on,vectors="+std::to_string(vm.cores*2+2));
                break;
            default: break;
        }
    }

    // ── IOMMU (QEMU 11) ───────────────────────────────────────────────
    switch(vm.iommu) {
        case IommuType::Intel:
            add("-device"); add("intel-iommu,intremap=on,caching-mode=on"); break;
        case IommuType::SMMUv3:
            add("-device"); add("arm-smmuv3"); break;
        case IommuType::VirtIO_IOMMU:
            add("-device"); add("virtio-iommu-pci"); break;
        default: break;
    }

    // ── TPM ───────────────────────────────────────────────────────────
    if(vm.tpm==TpmType::TIS) {
        add("-chardev"); add("socket,id=chrtpm,path=/tmp/swtpm.sock");
        add("-tpmdev");  add("emulator,id=tpm0,chardev=chrtpm");
        add("-device");  add("tpm-tis,tpmdev=tpm0");
    } else if(vm.tpm==TpmType::CRB) {
        add("-chardev"); add("socket,id=chrtpm,path=/tmp/swtpm.sock");
        add("-tpmdev");  add("emulator,id=tpm0,chardev=chrtpm");
        add("-device");  add("tpm-crb,tpmdev=tpm0");
    }

    // ── Confidential VM (QEMU 11 KVM) ─────────────────────────────────
    if(vm.conf_vm==ConfidentialVM::SEV_SNP) {
        add("-object"); add("sev-snp-guest,id=sev0,cbitpos=51,reduced-phys-bits=1");
        add("-machine"); add("memory-encryption=sev0");
    } else if(vm.conf_vm==ConfidentialVM::TDX) {
        add("-object"); add("tdx-guest,id=tdx0");
        add("-machine"); add("confidential-guest-support=tdx0");
    }

    // ── RISC-V IOMMU (QEMU 10) ────────────────────────────────────────
    if(vm.riscv_iommu && vm.arch==VMArch::riscv64) {
        add("-device"); add("riscv-iommu-sys");
    }

    // ── Misc hardware ─────────────────────────────────────────────────
    if(vm.virtio_rng){ add("-device"); add("virtio-rng-pci"); }

    // ── Misc flags ────────────────────────────────────────────────────
    if(vm.snapshot_mode) add("-snapshot");
    if(vm.no_reboot)     add("-no-reboot");

    // ── Extra args ────────────────────────────────────────────────────
    if(!vm.extra_args.empty()) {
        std::istringstream ss(vm.extra_args);
        std::string tok; while(ss>>tok) add(tok);
    }

    return a;
}

// ── Human-readable preview ────────────────────────────────────────────
std::string CommandBuilder::buildPreview(const VMConfig& vm) {
    std::string bin = resolveBinary(vm);
    auto args = buildArgs(vm);
    std::string out = bin;
    for(auto& a : args) {
        if(a.rfind('-',0)==0) out += " \\\n  "+a;
        else                  out += " "+a;
    }
    return out;
}

// ── Legacy wrappers ───────────────────────────────────────────────────
std::string CommandBuilder::buildCommand(const VMConfig& vm) {
    return buildPreview(vm);
}

std::string CommandBuilder::formatCommand(const VMConfig& vm) {
    return buildPreview(vm);
}

void CommandBuilder::writeStartScript(const VMConfig& vm) {
    if(vm.vm_dir.empty()) return;
    std::string path = vm.vm_dir + "/start.sh";
    std::ofstream f(path);
    if(!f) return;
    f << "#!/usr/bin/env bash\n# QEMU Manager — generated start script\n# VM: " << vm.name << "\n\n";
    std::string bin = resolveBinary(vm);
    auto args = buildArgs(vm);
    f << bin;
    for(auto& a : args) {
        if(a.rfind('-',0)==0) f << " \\\n  " << a;
        else                  f << " " << a;
    }
    f << "\n";
    f.close();
    // Make executable
    std::string cmd = "chmod +x \"" + path + "\"";
    system(cmd.c_str());
}

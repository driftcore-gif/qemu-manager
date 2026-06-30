#include "vm.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <sys/stat.h>

static std::string esc(const std::string& s) {
    std::string o; for(char c:s){ if(c=='"')o+="\\\""; else if(c=='\\')o+="\\\\"; else if(c=='\n')o+="\\n"; else o+=c; } return o;
}

// ── Enum → string ─────────────────────────────────────────────────────
static const char* archStr(VMArch a){
    static const char* t[]={"x86_64","i386","aarch64","arm","armeb","riscv64","riscv32","mips","mipsel","mips64","mips64el","mipsn32","mipsn32el","ppc","ppc64","ppc64le","sparc","sparc64","sparc32plus","alpha","hppa","m68k","microblaze","microblazeel","or1k","loongarch64","s390x","sh4","sh4eb","xtensa","xtensaeb","tricore","rx","avr","hexagon"};
    int i=(int)a; return(i>=0&&i<35)?t[i]:"x86_64";
}
static const char* machStr(MachineType m){
    static const char* t[]={"pc","q35","virt","microvm","sbsa_ref","virt_acpi","x86_64_microvm","nitro_enclave","custom"};
    return t[(int)m<9?(int)m:1];
}
static const char* dispStr(DisplayType d){
    static const char* t[]={"gtk","sdl","spice","vnc","egl","dbus","headless"};
    return t[(int)d<7?(int)d:0];
}
static const char* gpuStr(GPUType g){
    static const char* t[]={"VGA","VirtIO_GPU","VirtIO_GPU_GL","VirtIO_GPU_Rutabaga","VirtIO_GPU_NativeCtx","QXL","Cirrus","VMwareSVGA","ramfb","None"};
    return t[(int)g<10?(int)g:0];
}
static const char* audioStr(AudioType a){
    static const char* t[]={"IntelHDA","AC97","SB16","VirtIO_Sound","None"};
    return t[(int)a<5?(int)a:4];
}
static const char* netStr(NetworkMode n){
    static const char* t[]={"User","TAP","Bridge","Socket","VDE","VirtIO_VHostNet"};
    return t[(int)n<6?(int)n:0];
}
static const char* accelStr(Accelerator a){
    static const char* t[]={"TCG","KVM","KVM_LBT","WHPX","HVF","NVMM","Xen","Nitro","MSHV"};
    return t[(int)a<9?(int)a:0];
}
static const char* tbStr(TbSize t){
    static const char* s[]={"MB64","MB128","MB256","MB512","MB1024"};
    return s[(int)t<5?(int)t:2];
}
static const char* fmtStr(DiskFormat f){
    static const char* t[]={"qcow2","raw","vmdk","vdi"};
    return t[(int)f<4?(int)f:0];
}
static const char* ifaceStr(DiskInterface i){
    static const char* t[]={"virtio","scsi","nvme","ide","sata"};
    return t[(int)i<5?(int)i:0];
}
static const char* firmStr(Firmware f){
    static const char* t[]={"SeaBIOS","OVMF","OVMF_SecureBoot","U_Boot","EDK2_ARM"};
    return t[(int)f<5?(int)f:0];
}
static const char* usbVerStr(UsbVersion u){
    return (u==UsbVersion::USB3_XHCI)?"USB3_XHCI":"USB2_EHCI";
}
static const char* iommuStr(IommuType i){
    static const char* t[]={"None","Intel","SMMUv3","VirtIO_IOMMU"};
    return t[(int)i<4?(int)i:0];
}
static const char* tpmStr(TpmType t){
    static const char* s[]={"None","TIS","CRB"};
    return s[(int)t<3?(int)t:0];
}

// ── Serialize ─────────────────────────────────────────────────────────
std::string vmToJson(const VMConfig& vm) {
    std::ostringstream j;
    j<<"{\n";
    j<<"  \"name\": \""<<esc(vm.name)<<"\",\n";
    j<<"  \"description\": \""<<esc(vm.description)<<"\",\n";
    j<<"  \"arch\": \""<<archStr(vm.arch)<<"\",\n";
    j<<"  \"mode\": \""<<(vm.mode==VMMode::System?"System":"UserMode")<<"\",\n";
    j<<"  \"machine\": \""<<machStr(vm.machine)<<"\",\n";
    j<<"  \"machine_custom\": \""<<esc(vm.machine_custom)<<"\",\n";
    j<<"  \"cpu_model\": \""<<esc(vm.cpu_model)<<"\",\n";
    j<<"  \"cpu_flags\": \""<<esc(vm.cpu_flags)<<"\",\n";
    j<<"  \"cpu_migratable\": "<<(vm.cpu_migratable?"true":"false")<<",\n";
    j<<"  \"sockets\": "<<vm.sockets<<",\n";
    j<<"  \"cores\": "<<vm.cores<<",\n";
    j<<"  \"threads\": "<<vm.threads<<",\n";
    j<<"  \"ram_mb\": "<<vm.ram_mb<<",\n";
    j<<"  \"ballooning\": "<<(vm.ballooning?"true":"false")<<",\n";
    j<<"  \"memfd_backend\": "<<(vm.memfd_backend?"true":"false")<<",\n";
    j<<"  \"mem_slots\": "<<vm.mem_slots<<",\n";
    j<<"  \"disk_path\": \""<<esc(vm.disk_path)<<"\",\n";
    j<<"  \"iso_path\": \""<<esc(vm.iso_path)<<"\",\n";
    j<<"  \"disk2_path\": \""<<esc(vm.disk2_path)<<"\",\n";
    j<<"  \"disk_format\": \""<<fmtStr(vm.disk_format)<<"\",\n";
    j<<"  \"disk_interface\": \""<<ifaceStr(vm.disk_interface)<<"\",\n";
    j<<"  \"disk_size_gb\": "<<vm.disk_size_gb<<",\n";
    j<<"  \"disk_discard\": "<<(vm.disk_discard?"true":"false")<<",\n";
    j<<"  \"use_scsi_ctrl\": "<<(vm.use_scsi_ctrl?"true":"false")<<",\n";
    j<<"  \"nvme_namespaces\": "<<vm.nvme_namespaces<<",\n";
    j<<"  \"firmware\": \""<<firmStr(vm.firmware)<<"\",\n";
    j<<"  \"boot_menu\": "<<(vm.boot_menu?"true":"false")<<",\n";
    j<<"  \"boot_order\": \""<<esc(vm.boot_order)<<"\",\n";
    j<<"  \"secure_boot\": "<<(vm.secure_boot?"true":"false")<<",\n";
    j<<"  \"display\": \""<<dispStr(vm.display)<<"\",\n";
    j<<"  \"gpu\": \""<<gpuStr(vm.gpu)<<"\",\n";
    j<<"  \"audio\": \""<<audioStr(vm.audio)<<"\",\n";
    j<<"  \"resolution\": \""<<esc(vm.resolution)<<"\",\n";
    j<<"  \"virgl_enabled\": "<<(vm.virgl_enabled?"true":"false")<<",\n";
  j<<"  \"gpu_extra_outputs\": \"" <<esc(vm.gpu_extra_outputs)<<"\"," <<"\n";
  j<<"  \"kvm_cet\": "<<(vm.kvm_cet?"true":"false")<<",\n";
  j<<"  \"x86_cpu_gen\": \"" <<esc(vm.x86_cpu_gen)<<"\"," <<"\n";
  j<<"  \"conf_vm\": \"" <<(vm.conf_vm==ConfidentialVM::SEV_SNP?"SEV_SNP":vm.conf_vm==ConfidentialVM::TDX?"TDX":"None")<<"\"," <<"\n";
  j<<"  \"scsi_multiqueue\": "<<(vm.scsi_multiqueue?"true":"false")<<",\n";
  j<<"  \"riscv_iommu\": "<<(vm.riscv_iommu?"true":"false")<<",\n";
    j<<"  \"usb_version\": \""<<usbVerStr(vm.usb_version)<<"\",\n";
    j<<"  \"usb_tablet\": "<<(vm.usb_tablet?"true":"false")<<",\n";
    j<<"  \"net_mode\": \""<<netStr(vm.net_mode)<<"\",\n";
    j<<"  \"net_dns\": \""<<esc(vm.net_dns)<<"\",\n";
    j<<"  \"smb_share\": \""<<esc(vm.smb_share)<<"\",\n";
    j<<"  \"iommu\": \""<<iommuStr(vm.iommu)<<"\",\n";
    j<<"  \"tpm\": \""<<tpmStr(vm.tpm)<<"\",\n";
    j<<"  \"virtio_rng\": "<<(vm.virtio_rng?"true":"false")<<",\n";
    j<<"  \"numa_enabled\": "<<(vm.numa_enabled?"true":"false")<<",\n";
    j<<"  \"binary_mode\": \""<<(vm.binary_mode==BinaryMode::Auto?"Auto":"Custom")<<"\",\n";
    j<<"  \"custom_binary\": \""<<esc(vm.custom_binary)<<"\",\n";
    j<<"  \"accel\": \""<<accelStr(vm.accel)<<"\",\n";
    j<<"  \"tb_size\": \""<<tbStr(vm.tb_size)<<"\",\n";
    j<<"  \"kvm_nested\": "<<(vm.kvm_nested?"true":"false")<<",\n";
    j<<"  \"tcg_mttcg\": "<<(vm.tcg_mttcg?"true":"false")<<",\n";
    j<<"  \"snapshot_mode\": "<<(vm.snapshot_mode?"true":"false")<<",\n";
    j<<"  \"no_reboot\": "<<(vm.no_reboot?"true":"false")<<",\n";
    j<<"  \"extra_args\": \""<<esc(vm.extra_args)<<"\",\n";
    j<<"  \"save_script\": "<<(vm.save_script_to_vm_folder?"true":"false")<<",\n";
    j<<"  \"status\": \"Stopped\",\n";
    j<<"  \"vm_dir\": \""<<esc(vm.vm_dir)<<"\",\n";
    j<<"  \"last_started\": \""<<esc(vm.last_started)<<"\"\n";
    j<<"}\n";
    return j.str();
}

// ── Deserialize (hand-rolled minimal parser) ─────────────────────────
static std::string jsonStr(const std::string& s, const std::string& key) {
    std::string kp="\""+key+"\": \"";
    auto p=s.find(kp); if(p==std::string::npos)return "";
    p+=kp.size(); auto e=s.find('"',p);
    std::string r; for(size_t i=p;i<e;i++){
        if(s[i]=='\\'&&i+1<e){char c=s[++i];if(c=='"')r+='"';else if(c=='n')r+='\n';else r+=c;}
        else r+=s[i];
    }
    return r;
}
static int jsonInt(const std::string& s, const std::string& k, int def=0){
    std::string kp="\""+k+"\": ";
    auto p=s.find(kp); if(p==std::string::npos)return def;
    return std::stoi(s.substr(p+kp.size()));
}
static bool jsonBool(const std::string& s, const std::string& k, bool def=false){
    std::string kp="\""+k+"\": ";
    auto p=s.find(kp); if(p==std::string::npos)return def;
    std::string v=s.substr(p+kp.size(),5);
    return v.rfind("true",0)==0;
}

VMConfig vmFromJson(const std::string& s) {
    VMConfig vm;
    vm.name=jsonStr(s,"name"); vm.description=jsonStr(s,"description");
    // arch
    std::string arch=jsonStr(s,"arch");
    static const char* archs[]={"x86_64","i386","aarch64","arm","armeb","riscv64","riscv32","mips","mipsel","mips64","mips64el","mipsn32","mipsn32el","ppc","ppc64","ppc64le","sparc","sparc64","sparc32plus","alpha","hppa","m68k","microblaze","microblazeel","or1k","loongarch64","s390x","sh4","sh4eb","xtensa","xtensaeb","tricore","rx","avr","hexagon"};
    for(int i=0;i<35;i++){if(arch==archs[i]){vm.arch=(VMArch)i;break;}}
    vm.mode=jsonStr(s,"mode")=="UserMode"?VMMode::UserMode:VMMode::System;
    // machine
    std::string mach=jsonStr(s,"machine");
    static const char* machs[]={"pc","q35","virt","microvm","sbsa_ref","virt_acpi","x86_64_microvm","custom"};
    vm.machine=MachineType::q35;
    for(int i=0;i<8;i++){if(mach==machs[i]){vm.machine=(MachineType)i;break;}}
    vm.machine_custom=jsonStr(s,"machine_custom");
    vm.cpu_model=jsonStr(s,"cpu_model"); if(vm.cpu_model.empty())vm.cpu_model="max";
    vm.cpu_flags=jsonStr(s,"cpu_flags"); vm.cpu_migratable=jsonBool(s,"cpu_migratable",true);
    vm.sockets=jsonInt(s,"sockets",1); vm.cores=jsonInt(s,"cores",2); vm.threads=jsonInt(s,"threads",2);
    vm.ram_mb=jsonInt(s,"ram_mb",2048); vm.ballooning=jsonBool(s,"ballooning");
    vm.kvm_cet=jsonBool(s,"kvm_cet"); vm.x86_cpu_gen=jsonStr(s,"x86_cpu_gen");
    vm.scsi_multiqueue=jsonBool(s,"scsi_multiqueue"); vm.riscv_iommu=jsonBool(s,"riscv_iommu");
    vm.gpu_extra_outputs=jsonStr(s,"gpu_extra_outputs");
    std::string cvm=jsonStr(s,"conf_vm");
    if(cvm=="SEV_SNP") vm.conf_vm=ConfidentialVM::SEV_SNP;
    else if(cvm=="TDX") vm.conf_vm=ConfidentialVM::TDX;
    else vm.conf_vm=ConfidentialVM::None;
    vm.memfd_backend=jsonBool(s,"memfd_backend"); vm.mem_slots=jsonInt(s,"mem_slots",0);
    vm.disk_path=jsonStr(s,"disk_path"); vm.iso_path=jsonStr(s,"iso_path"); vm.disk2_path=jsonStr(s,"disk2_path");
    // disk format
    std::string df=jsonStr(s,"disk_format");
    if(df=="raw")vm.disk_format=DiskFormat::raw; else if(df=="vmdk")vm.disk_format=DiskFormat::vmdk;
    else if(df=="vdi")vm.disk_format=DiskFormat::vdi; else vm.disk_format=DiskFormat::qcow2;
    // disk interface
    std::string di=jsonStr(s,"disk_interface");
    if(di=="scsi")vm.disk_interface=DiskInterface::scsi; else if(di=="nvme")vm.disk_interface=DiskInterface::nvme;
    else if(di=="ide")vm.disk_interface=DiskInterface::ide; else if(di=="sata")vm.disk_interface=DiskInterface::sata;
    else vm.disk_interface=DiskInterface::virtio;
    vm.disk_size_gb=jsonInt(s,"disk_size_gb",20); vm.disk_discard=jsonBool(s,"disk_discard",true);
    vm.use_scsi_ctrl=jsonBool(s,"use_scsi_ctrl"); vm.nvme_namespaces=jsonInt(s,"nvme_namespaces",1);
    // firmware — support old "uefi" field
    std::string firm=jsonStr(s,"firmware");
    if(firm=="OVMF")vm.firmware=Firmware::OVMF; else if(firm=="OVMF_SecureBoot")vm.firmware=Firmware::OVMF_SecureBoot;
    else if(firm=="U_Boot")vm.firmware=Firmware::U_Boot; else if(firm=="EDK2_ARM")vm.firmware=Firmware::EDK2_ARM;
    else { vm.firmware=Firmware::SeaBIOS; if(jsonBool(s,"uefi"))vm.firmware=Firmware::OVMF; }
    vm.boot_menu=jsonBool(s,"boot_menu"); vm.boot_order=jsonStr(s,"boot_order"); if(vm.boot_order.empty())vm.boot_order="cd";
    vm.secure_boot=jsonBool(s,"secure_boot");
    // display
    std::string disp=jsonStr(s,"display");
    if(disp=="sdl")vm.display=DisplayType::SDL; else if(disp=="spice")vm.display=DisplayType::SPICE;
    else if(disp=="vnc")vm.display=DisplayType::VNC; else if(disp=="egl")vm.display=DisplayType::EGL;
    else if(disp=="dbus")vm.display=DisplayType::DBus; else if(disp=="headless")vm.display=DisplayType::Headless;
    else vm.display=DisplayType::GTK;
    // gpu
    std::string gpu=jsonStr(s,"gpu");
    if(gpu=="VirtIO_GPU")vm.gpu=GPUType::VirtIO_GPU; else if(gpu=="VirtIO_GPU_GL")vm.gpu=GPUType::VirtIO_GPU_GL;
    else if(gpu=="VirtIO_GPU_Rutabaga")vm.gpu=GPUType::VirtIO_GPU_Rutabaga;
    else if(gpu=="QXL")vm.gpu=GPUType::QXL; else if(gpu=="Cirrus")vm.gpu=GPUType::Cirrus;
    else if(gpu=="VMwareSVGA")vm.gpu=GPUType::VMwareSVGA; else if(gpu=="ramfb")vm.gpu=GPUType::ramfb;
    else if(gpu=="None")vm.gpu=GPUType::None; else vm.gpu=GPUType::VirtIO_GPU;
    // audio
    std::string aud=jsonStr(s,"audio");
    if(aud=="AC97")vm.audio=AudioType::AC97; else if(aud=="SB16")vm.audio=AudioType::SB16;
    else if(aud=="VirtIO_Sound")vm.audio=AudioType::VirtIO_Sound; else if(aud=="None")vm.audio=AudioType::None;
    else vm.audio=AudioType::IntelHDA;
    vm.resolution=jsonStr(s,"resolution"); if(vm.resolution.empty())vm.resolution="1280x720";
    vm.virgl_enabled=jsonBool(s,"virgl_enabled");
    vm.usb_version=jsonStr(s,"usb_version")=="USB2_EHCI"?UsbVersion::USB2_EHCI:UsbVersion::USB3_XHCI;
    vm.usb_tablet=jsonBool(s,"usb_tablet",true);
    // net
    std::string net=jsonStr(s,"net_mode");
    if(net=="TAP")vm.net_mode=NetworkMode::TAP; else if(net=="Bridge")vm.net_mode=NetworkMode::Bridge;
    else if(net=="Socket")vm.net_mode=NetworkMode::Socket; else if(net=="VDE")vm.net_mode=NetworkMode::VDE;
    else if(net=="VirtIO_VHostNet")vm.net_mode=NetworkMode::VirtIO_VHostNet; else vm.net_mode=NetworkMode::User;
    vm.net_dns=jsonStr(s,"net_dns"); vm.smb_share=jsonStr(s,"smb_share");
    // iommu
    std::string iommu=jsonStr(s,"iommu");
    if(iommu=="Intel")vm.iommu=IommuType::Intel; else if(iommu=="SMMUv3")vm.iommu=IommuType::SMMUv3;
    else if(iommu=="VirtIO_IOMMU")vm.iommu=IommuType::VirtIO_IOMMU; else vm.iommu=IommuType::None;
    // tpm
    std::string tpm=jsonStr(s,"tpm");
    if(tpm=="TIS")vm.tpm=TpmType::TIS; else if(tpm=="CRB")vm.tpm=TpmType::CRB; else vm.tpm=TpmType::None;
    vm.virtio_rng=jsonBool(s,"virtio_rng"); vm.numa_enabled=jsonBool(s,"numa_enabled");
    vm.binary_mode=jsonStr(s,"binary_mode")=="Custom"?BinaryMode::Custom:BinaryMode::Auto;
    vm.custom_binary=jsonStr(s,"custom_binary");
    // accel
    std::string acc=jsonStr(s,"accel");
    if(acc=="KVM")vm.accel=Accelerator::KVM; else if(acc=="KVM_LBT")vm.accel=Accelerator::KVM_LBT;
    else if(acc=="WHPX")vm.accel=Accelerator::WHPX; else if(acc=="HVF")vm.accel=Accelerator::HVF;
    else if(acc=="NVMM")vm.accel=Accelerator::NVMM; else if(acc=="Xen")vm.accel=Accelerator::Xen;
    else vm.accel=Accelerator::TCG;
    // tb_size
    std::string tb=jsonStr(s,"tb_size");
    if(tb=="MB64")vm.tb_size=TbSize::MB64; else if(tb=="MB128")vm.tb_size=TbSize::MB128;
    else if(tb=="MB512")vm.tb_size=TbSize::MB512; else if(tb=="MB1024")vm.tb_size=TbSize::MB1024;
    else vm.tb_size=TbSize::MB256;
    vm.kvm_nested=jsonBool(s,"kvm_nested"); vm.tcg_mttcg=jsonBool(s,"tcg_mttcg",true);
    vm.snapshot_mode=jsonBool(s,"snapshot_mode"); vm.no_reboot=jsonBool(s,"no_reboot");
    vm.extra_args=jsonStr(s,"extra_args"); vm.save_script_to_vm_folder=jsonBool(s,"save_script");
    std::string st=jsonStr(s,"status");
    if(st=="Running")vm.status=VMStatus::Running; else if(st=="Paused")vm.status=VMStatus::Paused;
    else vm.status=VMStatus::Stopped;
    vm.vm_dir=jsonStr(s,"vm_dir"); vm.last_started=jsonStr(s,"last_started");
    return vm;
}

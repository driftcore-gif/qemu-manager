#include "vmconfig_io.h"
#include "commandbuilder.h"
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>

std::string VMConfigIO::defaultVMDir() {
    const char* home = getenv("HOME");
    if (!home) home = "/tmp";
    return std::string(home) + "/.local/share/qemu-manager/VMs";
}

bool VMConfigIO::createVMDir(const VMConfig& vm) {
    if (vm.vm_dir.empty()) return false;
    mkdir(vm.vm_dir.c_str(), 0755);
    mkdir((vm.vm_dir+"/snapshots").c_str(), 0755);
    mkdir((vm.vm_dir+"/logs").c_str(), 0755);
    return true;
}

bool VMConfigIO::save(const VMConfig& vm) {
    createVMDir(vm);
    std::ofstream f(vm.vm_dir + "/vm.json");
    if (!f) return false;
    f << vmToJson(vm);
    return true;
}

VMConfig VMConfigIO::load(const std::string& vm_dir) {
    std::ifstream f(vm_dir + "/vm.json");
    if (!f) return {};
    std::ostringstream ss; ss << f.rdbuf();
    VMConfig vm = vmFromJson(ss.str());
    if (vm.vm_dir.empty()) vm.vm_dir = vm_dir;
    return vm;
}

std::vector<VMConfig> VMConfigIO::loadAll(const std::string& base_dir) {
    std::vector<VMConfig> vms;
    DIR* d = opendir(base_dir.c_str());
    if (!d) return vms;
    struct dirent* e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        std::string sub = base_dir + "/" + e->d_name;
        if (access((sub+"/vm.json").c_str(), R_OK) == 0) {
            VMConfig vm = load(sub);
            if (!vm.name.empty()) vms.push_back(vm);
        }
    }
    closedir(d);
    std::sort(vms.begin(),vms.end(),[](const VMConfig& a,const VMConfig& b){ return a.name<b.name; });
    return vms;
}

void VMConfigIO::writeStartScript(const VMConfig& vm) {
    if (vm.vm_dir.empty()) return;
    std::string path = vm.vm_dir + "/start.sh";
    std::ofstream f(path);
    if (!f) return;
    f << "#!/bin/bash\n# QEMU Manager — generated start script\n# VM: " << vm.name << "\n\n";
    f << CommandBuilder::formatCommand(vm) << "\n";
    f.close();
    chmod(path.c_str(), 0755);
}

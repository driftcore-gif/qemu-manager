#pragma once
#include "vm.h"
#include <string>
#include <vector>

struct VMTemplate {
    std::string name;
    std::string icon;
    VMArch arch;
    MachineType machine;
    std::string cpu_model;
    int cores;
    int ram_mb;
    int disk_size_gb;
    bool uefi;
    bool secure_boot;
    DisplayType display;
    GPUType gpu;
    AudioType audio;
    NetworkMode net_mode;
};

std::vector<VMTemplate> getTemplates();
VMConfig templateToConfig(const VMTemplate& t, const std::string& vm_dir);

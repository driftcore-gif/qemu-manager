#pragma once
#include "vm.h"
#include <string>
#include <vector>

class CommandBuilder {
public:
    static std::string archToQemuBin(VMArch arch);
    static std::string buildCommand(const VMConfig& vm, const std::string& qemu_bin_dir = "");
    static std::vector<std::string> buildArgs(const VMConfig& vm);
    static std::string formatCommand(const VMConfig& vm, const std::string& qemu_bin_dir = "");
};

#pragma once
#include "vm.h"
#include <string>
#include <vector>

struct CommandBuilder {
    static std::string              archToQemuBin(VMArch arch, VMMode mode);
    static std::string              resolveBinary(const VMConfig& vm);
    static std::vector<std::string> listInstalledBinaries(VMMode mode);
    static std::string              validateCustomBinary(const std::string& name);
    static std::vector<std::string> buildArgs(const VMConfig& vm);
    static std::string              buildPreview(const VMConfig& vm);
    static std::string              buildCommand(const VMConfig& vm);
    static std::string              formatCommand(const VMConfig& vm);
    static void                     writeStartScript(const VMConfig& vm);
};

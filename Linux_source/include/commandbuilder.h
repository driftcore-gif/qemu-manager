#pragma once
#include "vm.h"
#include <string>
#include <vector>

class CommandBuilder {
public:
    // Arch -> default binary name (no path)
    static std::string archToQemuBin(VMArch arch, VMMode mode);

    // Resolve full binary path respecting BinaryMode
    static std::string resolveBinary(const VMConfig& vm);

    // Scan /usr/bin/ for installed qemu binaries
    static std::vector<std::string> listInstalledBinaries(VMMode mode = VMMode::System);

    // Validate a custom binary name — returns "" if OK, error string if not
    static std::string validateCustomBinary(const std::string& name);

    // Build argv list
    static std::vector<std::string> buildArgs(const VMConfig& vm);

    // Single-line command string
    static std::string buildCommand(const VMConfig& vm,
                                    const std::string& unused = "");

    // Multi-line pretty command (for preview / start.sh)
    static std::string formatCommand(const VMConfig& vm,
                                     const std::string& unused = "");

    // Launch: read vm.json -> build in RAM -> fork+exec
    static pid_t launchVM(const std::string& vm_dir,
                          const std::string& unused = "");

    // Write start.sh to vm_dir (only when save_script_to_vm_folder is true)
    static bool writeStartScript(const VMConfig& vm,
                                 const std::string& unused = "");
};

#pragma once
#include "vm.h"
#include <string>
#include <vector>

struct VMConfigIO {
    static bool                  save(const VMConfig& vm);
    static VMConfig              load(const std::string& vm_dir);
    static std::vector<VMConfig> loadAll(const std::string& base_dir);
    static bool                  createVMDir(const VMConfig& vm);
    static void                  writeStartScript(const VMConfig& vm);
    static std::string           defaultVMDir();
};

// Free functions for backward compat
std::string vmToJson(const VMConfig& vm);
VMConfig    vmFromJson(const std::string& json);

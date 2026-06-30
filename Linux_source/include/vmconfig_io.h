#pragma once
#include "vm.h"
#include <string>

// Serialise / deserialise VMConfig to/from JSON
// Uses a minimal hand-written parser (no external deps)
class VMConfigIO {
public:
    // Save config to <vm_dir>/vm.json
    static bool save(const VMConfig& vm);

    // Load config from <vm_dir>/vm.json — returns false if missing/corrupt
    static bool load(const std::string& vm_dir, VMConfig& out);

    // Low-level: parse from string, serialise to string
    static std::string toJson(const VMConfig& vm);
    static bool        fromJson(const std::string& json, VMConfig& out);
};

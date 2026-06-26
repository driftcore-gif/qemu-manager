#pragma once
#include "vm.h"
#include <string>
#include <vector>
#include <functional>

class StorageManager {
public:
    static bool createDisk(const std::string& path, DiskFormat fmt, int size_gb);
    static bool resizeDisk(const std::string& path, int new_size_gb);
    static bool convertDisk(const std::string& src, const std::string& dst, DiskFormat fmt);
    static bool cloneDisk(const std::string& src, const std::string& dst);
    static std::vector<std::string> listISOs(const std::string& folder);
    static bool verifyChecksum(const std::string& file, const std::string& expected_sha256);
    static std::string formatToStr(DiskFormat fmt);
    static long long getDiskSizeMB(const std::string& path);
};

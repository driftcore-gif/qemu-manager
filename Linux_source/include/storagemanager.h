#pragma once
#include "vm.h"
#include <string>
#include <vector>
#include <functional>

struct DiskInfo {
    std::string path;
    std::string format;
    long long   virtual_size_bytes = 0;
    long long   actual_size_bytes  = 0;
    std::string backing_file;
    bool        valid = false;
    std::string error;
};

class StorageManager {
public:
    // ── Disk creation ────────────────────────────────────────────────
    static bool createDisk(const std::string& path, DiskFormat fmt,
                           int size_gb, bool prealloc = false);

    // ── Disk operations ──────────────────────────────────────────────
    static bool resizeDisk  (const std::string& path, int new_size_gb);
    static bool convertDisk (const std::string& src,
                             const std::string& dst, DiskFormat fmt);
    static bool cloneDisk   (const std::string& src, const std::string& dst);

    // ── Inspection ───────────────────────────────────────────────────
    static DiskInfo  inspectDisk     (const std::string& path);
    static long long getDiskSizeMB   (const std::string& path);
    static bool      verifyChecksum  (const std::string& file,
                                      const std::string& expected_sha256);

    // ── Discovery ────────────────────────────────────────────────────
    static std::vector<std::string> listISOs       (const std::string& folder);
    static std::vector<std::string> scanDiskImages  (const std::string& folder);

    // ── Utilities ────────────────────────────────────────────────────
    static std::string formatToStr   (DiskFormat fmt);
    static std::string humanSize     (long long bytes);
    static std::string resolveQemuImg();
};

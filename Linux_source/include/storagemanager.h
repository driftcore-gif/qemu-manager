#pragma once
#include "vm.h"
#include <string>
#include <vector>
#include <functional>
#include <gtk/gtk.h>

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

    static void createDiskAsync(GtkWindow* parent, const std::string& path,
                                DiskFormat fmt, int size_gb, bool prealloc = false,
                                std::function<void(bool success, const std::string& path)> cb = nullptr);

    // ── Disk operations ──────────────────────────────────────────────
    static bool resizeDisk  (const std::string& path, int new_size_gb);
    static bool convertDisk (const std::string& src,
                             const std::string& dst, DiskFormat fmt);
    static void convertDiskAsync(GtkWindow* parent, const std::string& src,
                                 const std::string& dst, DiskFormat fmt,
                                 std::function<void(bool success, const std::string& dst_path)> cb = nullptr);
    static bool cloneDisk   (const std::string& src, const std::string& dst);

    // ── Interactive Flows (Wizard & Settings) ────────────────────────
    static void startCreateDiskFlow(GtkWindow* parent, DiskFormat default_fmt, int default_size_gb, bool prealloc,
                                   std::function<void(bool success, const std::string& path)> cb = nullptr);

    static void startConvertDiskFlow(GtkWindow* parent, const std::string& initial_src = "",
                                    std::function<void(bool success, const std::string& dst_path)> cb = nullptr);

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
    static DiskFormat  strToFormat   (const std::string& fmt_str);
    static std::string humanSize     (long long bytes);
    static std::string resolveQemuImg();
};

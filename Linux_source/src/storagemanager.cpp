#include "storagemanager.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <array>
#include <memory>

// ── Internal helpers ─────────────────────────────────────────────────

static std::string exec(const std::string& cmd) {
    std::array<char, 512> buf;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buf.data(), buf.size(), pipe.get()) != nullptr)
        result += buf.data();
    return result;
}

static std::string jsonStr(const std::string& json, const std::string& key) {
    // Extract "key": "value"
    auto idx = json.find("\"" + key + "\"");
    if (idx == std::string::npos) return "";
    auto colon = json.find(":", idx);
    if (colon == std::string::npos) return "";
    auto q1 = json.find("\"", colon + 1);
    if (q1 == std::string::npos) return "";
    auto q2 = json.find("\"", q1 + 1);
    if (q2 == std::string::npos) return "";
    return json.substr(q1 + 1, q2 - q1 - 1);
}

static long long jsonLL(const std::string& json, const std::string& key) {
    auto idx = json.find("\"" + key + "\"");
    if (idx == std::string::npos) return 0;
    auto colon = json.find(":", idx);
    if (colon == std::string::npos) return 0;
    size_t s = colon + 1;
    while (s < json.size() && json[s] == ' ') s++;
    if (s >= json.size() || (!isdigit(json[s]) && json[s] != '-')) return 0;
    return std::stoll(json.substr(s));
}

// ── Utilities ────────────────────────────────────────────────────────

std::string StorageManager::formatToStr(DiskFormat fmt) {
    switch (fmt) {
        case DiskFormat::qcow2: return "qcow2";
        case DiskFormat::raw:   return "raw";
        case DiskFormat::vmdk:  return "vmdk";
        case DiskFormat::vdi:   return "vdi";
        default:                return "qcow2";
    }
}

std::string StorageManager::humanSize(long long bytes) {
    if (bytes <= 0) return "0 B";
    const char* units[] = {"B","KB","MB","GB","TB"};
    double v = (double)bytes;
    int i = 0;
    while (v >= 1024.0 && i < 4) { v /= 1024.0; i++; }
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f %s", v, units[i]);
    return buf;
}

std::string StorageManager::resolveQemuImg() {
    const char* paths[] = {
        "/usr/bin/qemu-img",
        "/usr/local/bin/qemu-img",
        "/data/data/com.termux/files/usr/bin/qemu-img",
        nullptr
    };
    for (int i = 0; paths[i]; i++) {
        struct stat st;
        if (stat(paths[i], &st) == 0) return paths[i];
    }
    return "qemu-img"; // rely on PATH
}

// ── Disk creation ─────────────────────────────────────────────────────

bool StorageManager::createDisk(const std::string& path, DiskFormat fmt,
                                 int size_gb, bool prealloc) {
    std::string qimg = resolveQemuImg();
    std::string f    = formatToStr(fmt);
    std::string cmd  = qimg + " create -f " + f;

    if (f == "qcow2" && size_gb >= 64)
        cmd += " -o cluster_size=2M,compression_type=zstd";
    else if (f == "raw" && prealloc)
        cmd += " -o preallocation=full";

    cmd += " \"" + path + "\" " + std::to_string(size_gb) + "G 2>&1";
    std::string out = exec(cmd);
    return out.find("Formatting") != std::string::npos
        || out.find("formatt")    != std::string::npos
        || (!out.empty() && out.find("error") == std::string::npos
                         && out.find("Error") == std::string::npos);
}

// ── Disk operations ───────────────────────────────────────────────────

bool StorageManager::resizeDisk(const std::string& path, int new_size_gb) {
    std::string cmd = resolveQemuImg() + " resize \""
        + path + "\" " + std::to_string(new_size_gb) + "G 2>&1";
    return system(cmd.c_str()) == 0;
}

bool StorageManager::convertDisk(const std::string& src,
                                  const std::string& dst, DiskFormat fmt) {
    std::string cmd = resolveQemuImg() + " convert -O "
        + formatToStr(fmt) + " \"" + src + "\" \"" + dst + "\" 2>&1";
    return system(cmd.c_str()) == 0;
}

bool StorageManager::cloneDisk(const std::string& src, const std::string& dst) {
    std::string cmd = "cp --reflink=auto \"" + src + "\" \"" + dst
        + "\" 2>/dev/null || cp \"" + src + "\" \"" + dst + "\"";
    return system(cmd.c_str()) == 0;
}

// ── Inspection ────────────────────────────────────────────────────────

DiskInfo StorageManager::inspectDisk(const std::string& path) {
    DiskInfo info;
    info.path = path;
    std::string cmd = resolveQemuImg() + " info --output=json \""
        + path + "\" 2>&1";
    std::string out = exec(cmd);
    if (out.find("error") != std::string::npos ||
        out.find("Error") != std::string::npos ||
        out.empty()) {
        info.valid = false;
        info.error = out;
        return info;
    }
    info.format               = jsonStr(out, "format");
    info.virtual_size_bytes   = jsonLL (out, "virtual-size");
    info.actual_size_bytes    = jsonLL (out, "actual-size");
    info.backing_file         = jsonStr(out, "backing-filename");
    info.valid                = !info.format.empty();
    return info;
}

long long StorageManager::getDiskSizeMB(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return 0;
    return (long long)st.st_size / (1024 * 1024);
}

bool StorageManager::verifyChecksum(const std::string& file,
                                     const std::string& expected_sha256) {
    std::string cmd = "sha256sum \"" + file + "\" 2>/dev/null";
    std::string out = exec(cmd);
    return !out.empty() && out.find(expected_sha256) != std::string::npos;
}

// ── Discovery ─────────────────────────────────────────────────────────

std::vector<std::string> StorageManager::listISOs(const std::string& folder) {
    std::vector<std::string> isos;
    DIR* dir = opendir(folder.c_str());
    if (!dir) return isos;
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        std::string name = ent->d_name;
        if (name.size() > 4 &&
            name.substr(name.size() - 4) == ".iso")
            isos.push_back(folder + "/" + name);
    }
    closedir(dir);
    return isos;
}

std::vector<std::string> StorageManager::scanDiskImages(const std::string& folder) {
    std::vector<std::string> found;
    static const char* exts[] = {".qcow2",".raw",".img",".vmdk",".vdi",nullptr};
    DIR* dir = opendir(folder.c_str());
    if (!dir) return found;
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        std::string n = ent->d_name;
        for (int i = 0; exts[i]; i++) {
            size_t el = strlen(exts[i]);
            if (n.size() > el &&
                n.substr(n.size() - el) == exts[i]) {
                found.push_back(folder + "/" + n);
                break;
            }
        }
    }
    closedir(dir);
    return found;
}

#include "storagemanager.h"
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <array>
#include <memory>
#include <stdexcept>

static std::string exec(const std::string& cmd) {
    std::array<char, 256> buf;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buf.data(), buf.size(), pipe.get()) != nullptr)
        result += buf.data();
    return result;
}

std::string StorageManager::formatToStr(DiskFormat fmt) {
    switch(fmt) {
        case DiskFormat::qcow2: return "qcow2";
        case DiskFormat::raw:   return "raw";
        case DiskFormat::vmdk:  return "vmdk";
        case DiskFormat::vdi:   return "vdi";
        default: return "qcow2";
    }
}

bool StorageManager::createDisk(const std::string& path, DiskFormat fmt, int size_gb) {
    std::string cmd = "qemu-img create -f " + formatToStr(fmt) +
        " \"" + path + "\" " + std::to_string(size_gb) + "G 2>&1";
    std::string out = exec(cmd);
    return out.find("Formatting") != std::string::npos || out.find("formatt") != std::string::npos || out.empty() == false;
}

bool StorageManager::resizeDisk(const std::string& path, int new_size_gb) {
    std::string cmd = "qemu-img resize \"" + path + "\" " + std::to_string(new_size_gb) + "G 2>&1";
    int ret = system(cmd.c_str());
    return ret == 0;
}

bool StorageManager::convertDisk(const std::string& src, const std::string& dst, DiskFormat fmt) {
    std::string cmd = "qemu-img convert -O " + formatToStr(fmt) +
        " \"" + src + "\" \"" + dst + "\" 2>&1";
    int ret = system(cmd.c_str());
    return ret == 0;
}

bool StorageManager::cloneDisk(const std::string& src, const std::string& dst) {
    std::string cmd = "cp --reflink=auto \"" + src + "\" \"" + dst + "\" 2>/dev/null || cp \"" + src + "\" \"" + dst + "\"";
    int ret = system(cmd.c_str());
    return ret == 0;
}

std::vector<std::string> StorageManager::listISOs(const std::string& folder) {
    std::vector<std::string> isos;
    DIR* dir = opendir(folder.c_str());
    if (!dir) return isos;
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        std::string name = ent->d_name;
        if (name.size() > 4 && name.substr(name.size()-4) == ".iso") {
            isos.push_back(folder + "/" + name);
        }
    }
    closedir(dir);
    return isos;
}

bool StorageManager::verifyChecksum(const std::string& file, const std::string& expected_sha256) {
    std::string cmd = "sha256sum \"" + file + "\" 2>/dev/null";
    std::string out = exec(cmd);
    return !out.empty() && out.find(expected_sha256) != std::string::npos;
}

long long StorageManager::getDiskSizeMB(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return 0;
    return (long long)st.st_size / (1024 * 1024);
}

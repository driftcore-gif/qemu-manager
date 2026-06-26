#include "snapshotmanager.h"
#include <cstdlib>
#include <sstream>
#include <array>
#include <memory>

static std::string exec_cmd(const std::string& cmd) {
    std::array<char,256> buf; std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(),"r"),pclose);
    if (!pipe) return "";
    while (fgets(buf.data(), buf.size(), pipe.get()) != nullptr) result += buf.data();
    return result;
}

bool SnapshotManager::createSnapshot(const std::string& disk_path, const std::string& name) {
    std::string cmd = "qemu-img snapshot -c \"" + name + "\" \"" + disk_path + "\" 2>&1";
    int ret = system(cmd.c_str());
    return ret == 0;
}

bool SnapshotManager::deleteSnapshot(const std::string& disk_path, const std::string& name) {
    std::string cmd = "qemu-img snapshot -d \"" + name + "\" \"" + disk_path + "\" 2>&1";
    int ret = system(cmd.c_str());
    return ret == 0;
}

bool SnapshotManager::restoreSnapshot(const std::string& disk_path, const std::string& name) {
    std::string cmd = "qemu-img snapshot -a \"" + name + "\" \"" + disk_path + "\" 2>&1";
    int ret = system(cmd.c_str());
    return ret == 0;
}

std::vector<std::string> SnapshotManager::listSnapshots(const std::string& disk_path) {
    std::string cmd = "qemu-img snapshot -l \"" + disk_path + "\" 2>/dev/null";
    std::string out = exec_cmd(cmd);
    std::vector<std::string> snaps;
    std::istringstream iss(out);
    std::string line;
    bool header_done = false;
    while (std::getline(iss, line)) {
        if (!header_done) { if (line.find("---") != std::string::npos) header_done = true; continue; }
        if (!line.empty()) snaps.push_back(line);
    }
    return snaps;
}

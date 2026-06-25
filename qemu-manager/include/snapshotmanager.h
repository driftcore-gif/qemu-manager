#pragma once
#include <string>
#include <vector>
#include <sstream>

class SnapshotManager {
public:
    static bool createSnapshot(const std::string& disk_path, const std::string& name);
    static bool deleteSnapshot(const std::string& disk_path, const std::string& name);
    static bool restoreSnapshot(const std::string& disk_path, const std::string& name);
    static std::vector<std::string> listSnapshots(const std::string& disk_path);
};

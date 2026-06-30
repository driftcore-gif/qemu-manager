#pragma once
#include <string>

struct AppSettings {
    std::string theme = "dark";
    std::string language = "en";
    bool auto_save = true;
    bool auto_update_check = false;
    std::string qemu_bin_path = "";
    std::string default_arch = "x86_64";
    std::string vm_folder;
    std::string iso_folder;
    std::string snapshot_folder;
    std::string default_args = "";
};

class Settings {
public:
    static AppSettings& get();
    static void load(const std::string& path);
    static void save(const std::string& path);
private:
    static AppSettings instance;
    static std::string config_path;
};

#include "settings.h"
#include <fstream>
#include <sstream>
#include <glib.h>

AppSettings Settings::instance;
std::string Settings::config_path;

AppSettings& Settings::get() { return instance; }

static std::string getVal(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < json.size() && (json[pos]==' '||json[pos]=='\t')) pos++;
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        size_t s = pos+1, e = json.find('"', s);
        return json.substr(s, e-s);
    }
    size_t e = json.find_first_of(",\n}", pos);
    return json.substr(pos, e-pos);
}

void Settings::load(const std::string& path) {
    config_path = path;
    std::string home = g_get_home_dir();
    instance.vm_folder = home + "/VMs";
    instance.iso_folder = home + "/ISOs";
    instance.snapshot_folder = home + "/Snapshots";

    std::ifstream f(path);
    if (!f.is_open()) return;
    std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    std::string v;
    v = getVal(json, "theme"); if (!v.empty()) instance.theme = v;
    v = getVal(json, "language"); if (!v.empty()) instance.language = v;
    v = getVal(json, "qemu_bin_path"); if (!v.empty()) instance.qemu_bin_path = v;
    v = getVal(json, "default_arch"); if (!v.empty()) instance.default_arch = v;
    v = getVal(json, "vm_folder"); if (!v.empty()) instance.vm_folder = v;
    v = getVal(json, "iso_folder"); if (!v.empty()) instance.iso_folder = v;
    v = getVal(json, "snapshot_folder"); if (!v.empty()) instance.snapshot_folder = v;
    v = getVal(json, "default_args"); if (!v.empty()) instance.default_args = v;
    v = getVal(json, "auto_save"); instance.auto_save = (v == "true");
    v = getVal(json, "auto_update_check"); instance.auto_update_check = (v == "true");
}

void Settings::save(const std::string& path) {
    std::ofstream f(path.empty() ? config_path : path);
    if (!f.is_open()) return;
    f << "{\n";
    f << "  \"theme\": \"" << instance.theme << "\",\n";
    f << "  \"language\": \"" << instance.language << "\",\n";
    f << "  \"auto_save\": " << (instance.auto_save?"true":"false") << ",\n";
    f << "  \"auto_update_check\": " << (instance.auto_update_check?"true":"false") << ",\n";
    f << "  \"qemu_bin_path\": \"" << instance.qemu_bin_path << "\",\n";
    f << "  \"default_arch\": \"" << instance.default_arch << "\",\n";
    f << "  \"vm_folder\": \"" << instance.vm_folder << "\",\n";
    f << "  \"iso_folder\": \"" << instance.iso_folder << "\",\n";
    f << "  \"snapshot_folder\": \"" << instance.snapshot_folder << "\",\n";
    f << "  \"default_args\": \"" << instance.default_args << "\"\n";
    f << "}\n";
}

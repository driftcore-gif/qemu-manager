#pragma once
#include <gtk/gtk.h>
#include "vm.h"
#include <vector>
#include <memory>
#include <functional>
#include <string>

class MainWindow {
public:
    MainWindow(GtkApplication* app);
    GtkWidget* getWidget() { return window; }

private:
    GtkWidget* window;
    GtkWidget* sidebar;
    GtkWidget* stack;
    GtkWidget* toolbar;
    GtkWidget* search_entry;
    GtkWidget* cmd_preview_label;
    GtkWidget* cmd_preview_box;
    std::vector<VMConfig> vms;
    std::string selected_vm_name;

    void buildUI(GtkApplication* app);
    void buildSidebar();
    void buildToolbar();
    void buildDashboardPage();
    void buildVMListPage();
    void buildStoragePage();
    void buildISOPage();
    void buildSnapshotPage();
    void buildTemplatesPage();
    void buildDownloadsPage();
    void buildLogsPage();
    void buildSettingsPage();
    void buildAboutPage();

    void loadVMs();
    void saveVM(const VMConfig& vm);
    void refreshVMList();
    void updateDashboard();

    // Actions
    void onNewVM();
    void onStartVM();
    void onStopVM();
    void onPauseVM();
    void onRestartVM();
    void onDeleteVM();
    void onCloneVM();
    void onImportVM();
    void onSettingsVM();
    void updateCommandPreview();

    // Static GTK callbacks
    static void onNewVMClicked(GtkButton*, gpointer data);
    static void onStartClicked(GtkButton*, gpointer data);
    static void onStopClicked(GtkButton*, gpointer data);
    static void onPauseClicked(GtkButton*, gpointer data);
    static void onRestartClicked(GtkButton*, gpointer data);
    static void onDeleteClicked(GtkButton*, gpointer data);
    static void onCloneClicked(GtkButton*, gpointer data);
    static void onImportClicked(GtkButton*, gpointer data);
    static void onSettingsClicked(GtkButton*, gpointer data);
    static void onSidebarRowActivated(GtkListBox*, GtkListBoxRow*, gpointer data);
    static void onSearchChanged(GtkSearchEntry*, gpointer data);
};

// ---- VM Settings/Edit Dialog ----
class VMSettingsDialog {
public:
    VMSettingsDialog(GtkWindow* parent, VMConfig& vm, std::function<void(VMConfig)> on_save);
    void show();
private:
    GtkWidget* dialog;
    GtkWidget* notebook;
    VMConfig& vm_ref;
    std::function<void(VMConfig)> on_save_cb;
    
    // General
    GtkWidget* name_entry, *desc_entry, *arch_combo, *mode_combo, *mach_combo, *mach_custom;
    // CPU
    GtkWidget* cpu_entry, *sockets_spin, *cores_spin, *threads_spin;
    // Memory
    GtkWidget* ram_spin, *balloon_check;
    // Storage
    GtkWidget* disk_size_spin, *disk_fmt_combo, *iso_entry;
    // Boot
    GtkWidget* uefi_check, *boot_menu_check, *boot_combo;
    // Display
    GtkWidget* display_combo, *gpu_combo, *audio_combo;
    // Network
    GtkWidget* net_combo;
    // Hardware
    GtkWidget* bin_combo, *custom_bin_entry, *bin_hint_lbl, *accel_combo, *lbt_note_lbl, *extra_args_entry, *save_script_check;
    GtkWidget* kvm_cet_check = nullptr, *conf_vm_combo = nullptr, *scsi_mq_check = nullptr, *riscv_iommu_check = nullptr;
    GtkWidget* io_uring_check = nullptr, *migration_combo = nullptr, *virtfs_path_entry = nullptr, *virtfs_tag_entry = nullptr;
    // Preview
    GtkWidget* cmd_preview;
    
    void buildPages();
    void updatePreview();
    VMConfig collectConfig();
    void populateFromVM();
    
    static void onSaveClicked(GtkButton*, gpointer);
    static void onCancelClicked(GtkButton*, gpointer);
    static void onBrowseISO(GtkButton*, gpointer);
};

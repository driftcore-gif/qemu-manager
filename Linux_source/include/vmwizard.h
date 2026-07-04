#pragma once
#include <gtk/gtk.h>
#include "vm.h"
#include <functional>
#include <string>

class VMWizard {
public:
    VMWizard(GtkWindow* parent, std::function<void(VMConfig)> on_create);
    void show();

private:
    GtkWidget* dialog   = nullptr;
    GtkWidget* notebook = nullptr;

    // General
    GtkWidget* name_entry   = nullptr;
    GtkWidget* desc_entry   = nullptr;
    GtkWidget* arch_combo   = nullptr;
    GtkWidget* mode_combo   = nullptr;
    GtkWidget* mach_combo   = nullptr;
    GtkWidget* mach_custom  = nullptr;

    // CPU
    GtkWidget* cpu_entry    = nullptr;
    GtkWidget* sockets_spin = nullptr;
    GtkWidget* cores_spin   = nullptr;
    GtkWidget* threads_spin = nullptr;

    // Memory
    GtkWidget* ram_spin      = nullptr;
    GtkWidget* balloon_check = nullptr;

    // Storage
    // disk creation mode
    GtkWidget* disk_mode_new      = nullptr;  // radio: create new
    GtkWidget* disk_mode_existing = nullptr;  // radio: use existing
    GtkWidget* disk_mode_none     = nullptr;  // radio: no disk
    GtkWidget* disk_new_box       = nullptr;  // shown when create-new selected
    GtkWidget* disk_existing_box  = nullptr;  // shown when existing selected
    GtkWidget* disk_size_spin     = nullptr;
    GtkWidget* disk_size_lbl      = nullptr;  // shows TB label for large values
    GtkWidget* disk_fmt_combo     = nullptr;
    GtkWidget* prealloc_check     = nullptr;
    GtkWidget* disk_status_lbl    = nullptr;  // qemu-img path / not found
    GtkWidget* existing_disk_entry= nullptr;  // path for existing disk
    GtkWidget* iso_entry          = nullptr;

    // Boot
    GtkWidget* uefi_check       = nullptr;
    GtkWidget* boot_menu_check  = nullptr;
    GtkWidget* boot_combo       = nullptr;

    // Display
    GtkWidget* display_combo = nullptr;
    GtkWidget* gpu_combo     = nullptr;
    GtkWidget* audio_combo   = nullptr;

    // Network
    GtkWidget* net_combo = nullptr;

    // Hardware page
    GtkWidget* bin_combo        = nullptr;  // dropdown: detected binaries + "Custom…"
    GtkWidget* custom_bin_entry = nullptr;  // visible only when Custom… selected
    GtkWidget* bin_hint_lbl     = nullptr;
    GtkWidget* accel_combo      = nullptr;  // TCG / KVM / KVM+LBT
    GtkWidget* lbt_note_lbl     = nullptr;
    GtkWidget* extra_args_entry = nullptr;
    GtkWidget* save_script_check= nullptr;

    // Preview
    GtkWidget* cmd_preview = nullptr;

    std::function<void(VMConfig)> on_create_cb;

    void      buildPages();
    void      updatePreview();
    VMConfig  collectConfig();

    GtkWidget* buildGeneralPage();
    GtkWidget* buildCPUPage();
    GtkWidget* buildMemoryPage();
    GtkWidget* buildStoragePage();
    GtkWidget* buildBootPage();
    GtkWidget* buildDisplayPage();
    GtkWidget* buildNetworkPage();
    GtkWidget* buildHardwarePage();   // binary + accelerator + extra args
    GtkWidget* buildPreviewPage();

    static void onBrowseISO(GtkButton*, gpointer);
    static void onBrowseDisk(GtkButton*, gpointer);
    static void onDiskModeChanged(GtkCheckButton*, gpointer);
    static void onDiskSizeChanged(GtkSpinButton*, gpointer);
    static void onCreateClicked(GtkButton*, gpointer);
    static void onCancelClicked(GtkButton*, gpointer);
    static void onPageSwitch(GtkNotebook*, GtkWidget*, guint, gpointer);
};

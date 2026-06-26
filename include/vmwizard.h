#pragma once
#include <gtk/gtk.h>
#include "vm.h"
#include <functional>

class VMWizard {
public:
    VMWizard(GtkWindow* parent, std::function<void(VMConfig)> on_create);
    void show();

private:
    GtkWidget* dialog;
    GtkWidget* notebook;
    VMConfig config;
    std::function<void(VMConfig)> on_create_cb;

    // Page widgets
    GtkWidget* name_entry;
    GtkWidget* desc_entry;
    GtkWidget* arch_combo;
    GtkWidget* machine_combo;
    GtkWidget* cpu_combo;
    GtkWidget* sockets_spin;
    GtkWidget* cores_spin;
    GtkWidget* threads_spin;
    GtkWidget* ram_spin;
    GtkWidget* disk_size_spin;
    GtkWidget* disk_fmt_combo;
    GtkWidget* iso_entry;
    GtkWidget* display_combo;
    GtkWidget* gpu_combo;
    GtkWidget* audio_combo;
    GtkWidget* net_combo;
    GtkWidget* uefi_check;
    GtkWidget* boot_menu_check;
    GtkWidget* extra_args_entry;
    GtkWidget* cmd_preview_label;

    void buildPages();
    GtkWidget* buildGeneralPage();
    GtkWidget* buildCPUPage();
    GtkWidget* buildMemoryPage();
    GtkWidget* buildStoragePage();
    GtkWidget* buildBootPage();
    GtkWidget* buildDisplayPage();
    GtkWidget* buildNetworkPage();
    GtkWidget* buildAdvancedPage();
    GtkWidget* buildPreviewPage();

    void updatePreview();
    VMConfig collectConfig();

    static void onCreateClicked(GtkButton*, gpointer data);
    static void onCancelClicked(GtkButton*, gpointer data);
    static void onBrowseISO(GtkButton*, gpointer data);
    static void onPageSwitch(GtkNotebook*, GtkWidget*, guint, gpointer data);
};

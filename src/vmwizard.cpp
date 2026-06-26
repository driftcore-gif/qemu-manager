#include "vmwizard.h"
#include "commandbuilder.h"
#include "storagemanager.h"
#include "settings.h"
#include <string>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <ctime>

VMWizard::VMWizard(GtkWindow* parent, std::function<void(VMConfig)> on_create)
    : on_create_cb(on_create) {

    dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "New Virtual Machine");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 700, 560);

    buildPages();
}

void VMWizard::show() {
    gtk_window_present(GTK_WINDOW(dialog));
}

GtkWidget* VMWizard::buildGeneralPage() {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 16);

    auto add_row = [&](int row, const char* label, GtkWidget* widget) {
        GtkWidget* lbl = gtk_label_new(label);
        gtk_label_set_xalign(GTK_LABEL(lbl), 1.0f);
        gtk_grid_attach(GTK_GRID(grid), lbl, 0, row, 1, 1);
        gtk_widget_set_hexpand(widget, TRUE);
        gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);
    };

    name_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(name_entry), "e.g. Ubuntu Desktop");
    add_row(0, "VM Name:", name_entry);

    desc_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(desc_entry), "Optional description");
    add_row(1, "Description:", desc_entry);

    arch_combo = gtk_drop_down_new_from_strings(
        (const char*[]){"x86_64","i386","aarch64","arm","riscv64","riscv32","mips","mipsel","ppc64","sparc",nullptr});
    add_row(2, "Architecture:", arch_combo);

    machine_combo = gtk_drop_down_new_from_strings(
        (const char*[]){"q35","pc","virt","microvm",nullptr});
    add_row(3, "Machine Type:", machine_combo);

    return grid;
}

GtkWidget* VMWizard::buildCPUPage() {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 16);

    cpu_combo = gtk_drop_down_new_from_strings(
        (const char*[]){"max","host","qemu64","Haswell","Broadwell","Skylake-Client","EPYC","cortex-a53","rv64",nullptr});
    GtkWidget* cpu_lbl = gtk_label_new("CPU Model:");
    gtk_label_set_xalign(GTK_LABEL(cpu_lbl), 1.0f);
    gtk_grid_attach(GTK_GRID(grid), cpu_lbl, 0, 0, 1, 1);
    gtk_widget_set_hexpand(cpu_combo, TRUE);
    gtk_grid_attach(GTK_GRID(grid), cpu_combo, 1, 0, 1, 1);

    auto add_spin = [&](int row, const char* label, GtkWidget** out, int val, int min, int max) {
        GtkWidget* lbl = gtk_label_new(label);
        gtk_label_set_xalign(GTK_LABEL(lbl), 1.0f);
        gtk_grid_attach(GTK_GRID(grid), lbl, 0, row, 1, 1);
        *out = gtk_spin_button_new_with_range(min, max, 1);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(*out), val);
        gtk_widget_set_hexpand(*out, TRUE);
        gtk_grid_attach(GTK_GRID(grid), *out, 1, row, 1, 1);
    };

    add_spin(1, "Sockets:", &sockets_spin, 1, 1, 8);
    add_spin(2, "Cores:", &cores_spin, 2, 1, 32);
    add_spin(3, "Threads:", &threads_spin, 2, 1, 8);

    return grid;
}

GtkWidget* VMWizard::buildMemoryPage() {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 16);

    GtkWidget* lbl = gtk_label_new("RAM (MB):");
    gtk_label_set_xalign(GTK_LABEL(lbl), 1.0f);
    gtk_grid_attach(GTK_GRID(grid), lbl, 0, 0, 1, 1);
    ram_spin = gtk_spin_button_new_with_range(128, 65536, 256);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ram_spin), 2048);
    gtk_widget_set_hexpand(ram_spin, TRUE);
    gtk_grid_attach(GTK_GRID(grid), ram_spin, 1, 0, 1, 1);

    GtkWidget* hint = gtk_label_new("Recommended: 2048 MB minimum for desktop OS.\nPRoot may have limited memory.");
    gtk_label_set_xalign(GTK_LABEL(hint), 0);
    gtk_widget_add_css_class(hint, "dim-label");
    gtk_label_set_wrap(GTK_LABEL(hint), TRUE);
    gtk_grid_attach(GTK_GRID(grid), hint, 0, 1, 2, 1);

    return grid;
}

GtkWidget* VMWizard::buildStoragePage() {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 16);

    GtkWidget* lbl1 = gtk_label_new("Disk Size (GB):");
    gtk_label_set_xalign(GTK_LABEL(lbl1), 1.0f);
    gtk_grid_attach(GTK_GRID(grid), lbl1, 0, 0, 1, 1);
    disk_size_spin = gtk_spin_button_new_with_range(1, 2000, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(disk_size_spin), 20);
    gtk_widget_set_hexpand(disk_size_spin, TRUE);
    gtk_grid_attach(GTK_GRID(grid), disk_size_spin, 1, 0, 1, 1);

    GtkWidget* lbl2 = gtk_label_new("Disk Format:");
    gtk_label_set_xalign(GTK_LABEL(lbl2), 1.0f);
    gtk_grid_attach(GTK_GRID(grid), lbl2, 0, 1, 1, 1);
    disk_fmt_combo = gtk_drop_down_new_from_strings(
        (const char*[]){"qcow2","raw","vmdk","vdi",nullptr});
    gtk_widget_set_hexpand(disk_fmt_combo, TRUE);
    gtk_grid_attach(GTK_GRID(grid), disk_fmt_combo, 1, 1, 1, 1);

    GtkWidget* lbl3 = gtk_label_new("ISO Path:");
    gtk_label_set_xalign(GTK_LABEL(lbl3), 1.0f);
    gtk_grid_attach(GTK_GRID(grid), lbl3, 0, 2, 1, 1);

    GtkWidget* iso_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    iso_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(iso_entry), "/path/to/file.iso");
    gtk_widget_set_hexpand(iso_entry, TRUE);
    GtkWidget* browse_btn = gtk_button_new_with_label("Browse");
    g_signal_connect(browse_btn, "clicked", G_CALLBACK(onBrowseISO), this);
    gtk_box_append(GTK_BOX(iso_box), iso_entry);
    gtk_box_append(GTK_BOX(iso_box), browse_btn);
    gtk_widget_set_hexpand(iso_box, TRUE);
    gtk_grid_attach(GTK_GRID(grid), iso_box, 1, 2, 1, 1);

    return grid;
}

GtkWidget* VMWizard::buildBootPage() {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 16);

    uefi_check = gtk_check_button_new_with_label("Enable UEFI (requires OVMF firmware)");
    gtk_grid_attach(GTK_GRID(grid), uefi_check, 0, 0, 2, 1);

    boot_menu_check = gtk_check_button_new_with_label("Show boot menu on startup");
    gtk_grid_attach(GTK_GRID(grid), boot_menu_check, 0, 1, 2, 1);

    GtkWidget* lbl = gtk_label_new("Boot Order:");
    gtk_label_set_xalign(GTK_LABEL(lbl), 1.0f);
    gtk_grid_attach(GTK_GRID(grid), lbl, 0, 2, 1, 1);
    GtkWidget* boot_combo = gtk_drop_down_new_from_strings(
        (const char*[]){"cd (CD first)","dc (disk first)","c (disk only)","d (CD only)",nullptr});
    gtk_widget_set_hexpand(boot_combo, TRUE);
    gtk_grid_attach(GTK_GRID(grid), boot_combo, 1, 2, 1, 1);

    return grid;
}

GtkWidget* VMWizard::buildDisplayPage() {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 16);

    GtkWidget* lbl1 = gtk_label_new("Display:");
    gtk_label_set_xalign(GTK_LABEL(lbl1), 1.0f);
    gtk_grid_attach(GTK_GRID(grid), lbl1, 0, 0, 1, 1);
    display_combo = gtk_drop_down_new_from_strings(
        (const char*[]){"GTK","SDL","SPICE","VNC","EGL","Headless",nullptr});
    gtk_widget_set_hexpand(display_combo, TRUE);
    gtk_grid_attach(GTK_GRID(grid), display_combo, 1, 0, 1, 1);

    GtkWidget* lbl2 = gtk_label_new("GPU:");
    gtk_label_set_xalign(GTK_LABEL(lbl2), 1.0f);
    gtk_grid_attach(GTK_GRID(grid), lbl2, 0, 1, 1, 1);
    gpu_combo = gtk_drop_down_new_from_strings(
        (const char*[]){"VGA","VirtIO GPU","QXL","Cirrus","VMware SVGA","None",nullptr});
    gtk_drop_down_set_selected(GTK_DROP_DOWN(gpu_combo), 1);
    gtk_widget_set_hexpand(gpu_combo, TRUE);
    gtk_grid_attach(GTK_GRID(grid), gpu_combo, 1, 1, 1, 1);

    GtkWidget* lbl3 = gtk_label_new("Audio:");
    gtk_label_set_xalign(GTK_LABEL(lbl3), 1.0f);
    gtk_grid_attach(GTK_GRID(grid), lbl3, 0, 2, 1, 1);
    audio_combo = gtk_drop_down_new_from_strings(
        (const char*[]){"Intel HDA","AC97","SB16","None",nullptr});
    gtk_widget_set_hexpand(audio_combo, TRUE);
    gtk_grid_attach(GTK_GRID(grid), audio_combo, 1, 2, 1, 1);

    return grid;
}

GtkWidget* VMWizard::buildNetworkPage() {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 16);

    GtkWidget* lbl = gtk_label_new("Network Mode:");
    gtk_label_set_xalign(GTK_LABEL(lbl), 1.0f);
    gtk_grid_attach(GTK_GRID(grid), lbl, 0, 0, 1, 1);
    net_combo = gtk_drop_down_new_from_strings(
        (const char*[]){"User (NAT - no root)","TAP","Bridge","Socket",nullptr});
    gtk_widget_set_hexpand(net_combo, TRUE);
    gtk_grid_attach(GTK_GRID(grid), net_combo, 1, 0, 1, 1);

    GtkWidget* info = gtk_label_new("User mode works without root in Termux/PRoot.\nTAP/Bridge may need additional setup.");
    gtk_widget_add_css_class(info, "dim-label");
    gtk_label_set_wrap(GTK_LABEL(info), TRUE);
    gtk_label_set_xalign(GTK_LABEL(info), 0);
    gtk_grid_attach(GTK_GRID(grid), info, 0, 1, 2, 1);

    return grid;
}

GtkWidget* VMWizard::buildAdvancedPage() {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 16);

    GtkWidget* lbl = gtk_label_new("Extra QEMU Args:");
    gtk_label_set_xalign(GTK_LABEL(lbl), 1.0f);
    gtk_grid_attach(GTK_GRID(grid), lbl, 0, 0, 1, 1);
    extra_args_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(extra_args_entry), "-accel kvm -enable-kvm");
    gtk_widget_set_hexpand(extra_args_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), extra_args_entry, 1, 0, 1, 1);

    GtkWidget* hint = gtk_label_new("Additional QEMU flags appended to the command.\nExample: -accel tcg,thread=multi for multithreaded TCG.");
    gtk_widget_add_css_class(hint, "dim-label");
    gtk_label_set_wrap(GTK_LABEL(hint), TRUE);
    gtk_label_set_xalign(GTK_LABEL(hint), 0);
    gtk_grid_attach(GTK_GRID(grid), hint, 0, 1, 2, 1);

    return grid;
}

GtkWidget* VMWizard::buildPreviewPage() {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 12);

    GtkWidget* lbl = gtk_label_new("Generated QEMU Command:");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(lbl), attrs);
    pango_attr_list_unref(attrs);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);

    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    cmd_preview_label = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(cmd_preview_label), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(cmd_preview_label), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(cmd_preview_label), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(cmd_preview_label), TRUE);
    gtk_widget_set_focusable(cmd_preview_label, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), cmd_preview_label);

    gtk_box_append(GTK_BOX(box), lbl);
    gtk_box_append(GTK_BOX(box), scroll);
    return box;
}

void VMWizard::updatePreview() {
    VMConfig c = collectConfig();
    std::string cmd = CommandBuilder::formatCommand(c, Settings::get().qemu_bin_path);
    GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(cmd_preview_label));
    gtk_text_buffer_set_text(buf, cmd.c_str(), -1);
}

VMConfig VMWizard::collectConfig() {
    VMConfig c;
    c.name = gtk_editable_get_text(GTK_EDITABLE(name_entry));
    c.description = gtk_editable_get_text(GTK_EDITABLE(desc_entry));

    int arch_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(arch_combo));
    c.arch = (VMArch)arch_idx;

    int mach_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(machine_combo));
    if (mach_idx == 1) c.machine = MachineType::pc;
    else if (mach_idx == 2) c.machine = MachineType::virt;
    else if (mach_idx == 3) c.machine = MachineType::microvm;
    else c.machine = MachineType::q35;

    const char* cpus[] = {"max","host","qemu64","Haswell","Broadwell","Skylake-Client","EPYC","cortex-a53","rv64"};
    int cpu_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(cpu_combo));
    if (cpu_idx < 9) c.cpu_model = cpus[cpu_idx];

    c.sockets = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(sockets_spin));
    c.cores   = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(cores_spin));
    c.threads = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(threads_spin));
    c.ram_mb  = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(ram_spin));
    c.disk_size_gb = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(disk_size_spin));

    int fmt_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(disk_fmt_combo));
    c.disk_format = (DiskFormat)fmt_idx;

    c.iso_path = gtk_editable_get_text(GTK_EDITABLE(iso_entry));
    c.uefi = gtk_check_button_get_active(GTK_CHECK_BUTTON(uefi_check));
    c.boot_menu = gtk_check_button_get_active(GTK_CHECK_BUTTON(boot_menu_check));
    c.display = (DisplayType)gtk_drop_down_get_selected(GTK_DROP_DOWN(display_combo));
    c.gpu = (GPUType)gtk_drop_down_get_selected(GTK_DROP_DOWN(gpu_combo));
    c.audio = (AudioType)gtk_drop_down_get_selected(GTK_DROP_DOWN(audio_combo));
    c.net_mode = (NetworkMode)gtk_drop_down_get_selected(GTK_DROP_DOWN(net_combo));
    c.extra_args = gtk_editable_get_text(GTK_EDITABLE(extra_args_entry));

    std::string vm_dir = Settings::get().vm_folder + "/" + c.name;
    c.vm_dir = vm_dir;
    const char* exts[] = {"qcow2","raw","vmdk","vdi"};
    c.disk_path = vm_dir + "/disk." + exts[fmt_idx < 4 ? fmt_idx : 0];

    return c;
}

// GTK4 alert dialog helper (replaces gtk_message_dialog / gtk_dialog_run)
static void show_alert(GtkWindow* parent, const std::string& msg) {
    GtkAlertDialog* dlg = gtk_alert_dialog_new("%s", msg.c_str());
    gtk_alert_dialog_show(dlg, parent);
    g_object_unref(dlg);
}

// GTK4 file dialog for ISO selection (async)
struct BrowseData { GtkEntry* entry; GtkWindow* parent; };

static void on_iso_file_open(GObject* source, GAsyncResult* res, gpointer user_data) {
    BrowseData* bd = (BrowseData*)user_data;
    GError* err = nullptr;
    GFile* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), res, &err);
    if (file) {
        char* path = g_file_get_path(file);
        if (path) {
            gtk_editable_set_text(GTK_EDITABLE(bd->entry), path);
            g_free(path);
        }
        g_object_unref(file);
    }
    if (err) g_error_free(err);
    delete bd;
}

void VMWizard::onBrowseISO(GtkButton*, gpointer data) {
    VMWizard* self = (VMWizard*)data;
    GtkFileDialog* dlg = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dlg, "Select ISO");

    GListStore* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.iso");
    gtk_file_filter_set_name(filter, "ISO Files (*.iso)");
    g_list_store_append(filters, filter);
    g_object_unref(filter);
    gtk_file_dialog_set_filters(dlg, G_LIST_MODEL(filters));
    g_object_unref(filters);

    BrowseData* bd = new BrowseData{GTK_ENTRY(self->iso_entry), GTK_WINDOW(self->dialog)};
    gtk_file_dialog_open(dlg, GTK_WINDOW(self->dialog), nullptr, on_iso_file_open, bd);
    g_object_unref(dlg);
}

void VMWizard::buildPages() {
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(dialog), main_box);

    GtkWidget* title_lbl = gtk_label_new("Create New Virtual Machine");
    PangoAttrList* ta = pango_attr_list_new();
    pango_attr_list_insert(ta, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(ta, pango_attr_scale_new(1.3));
    gtk_label_set_attributes(GTK_LABEL(title_lbl), ta);
    pango_attr_list_unref(ta);
    gtk_widget_set_margin_top(title_lbl, 16);
    gtk_widget_set_margin_bottom(title_lbl, 8);
    gtk_box_append(GTK_BOX(main_box), title_lbl);

    notebook = gtk_notebook_new();
    gtk_widget_set_vexpand(notebook, TRUE);
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

    auto add_page = [&](const char* label, GtkWidget* widget) {
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, gtk_label_new(label));
    };

    add_page("General",  buildGeneralPage());
    add_page("CPU",      buildCPUPage());
    add_page("Memory",   buildMemoryPage());
    add_page("Storage",  buildStoragePage());
    add_page("Boot",     buildBootPage());
    add_page("Display",  buildDisplayPage());
    add_page("Network",  buildNetworkPage());
    add_page("Advanced", buildAdvancedPage());
    add_page("Preview",  buildPreviewPage());

    g_signal_connect(notebook, "switch-page", G_CALLBACK(onPageSwitch), this);
    gtk_box_append(GTK_BOX(main_box), notebook);

    GtkWidget* btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(btn_box, 16);
    gtk_widget_set_margin_end(btn_box, 16);
    gtk_widget_set_margin_top(btn_box, 8);
    gtk_widget_set_margin_bottom(btn_box, 12);

    GtkWidget* spacer = gtk_label_new("");
    gtk_widget_set_hexpand(spacer, TRUE);
    GtkWidget* cancel_btn = gtk_button_new_with_label("Cancel");
    GtkWidget* create_btn = gtk_button_new_with_label("Create VM");
    gtk_widget_add_css_class(create_btn, "suggested-action");

    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(onCancelClicked), this);
    g_signal_connect(create_btn, "clicked", G_CALLBACK(onCreateClicked), this);

    gtk_box_append(GTK_BOX(btn_box), spacer);
    gtk_box_append(GTK_BOX(btn_box), cancel_btn);
    gtk_box_append(GTK_BOX(btn_box), create_btn);
    gtk_box_append(GTK_BOX(main_box), btn_box);
}

void VMWizard::onPageSwitch(GtkNotebook* nb, GtkWidget*, guint page_num, gpointer data) {
    VMWizard* self = (VMWizard*)data;
    int total = gtk_notebook_get_n_pages(nb);
    if ((int)page_num == total - 1) self->updatePreview();
}

void VMWizard::onCreateClicked(GtkButton*, gpointer data) {
    VMWizard* self = (VMWizard*)data;
    VMConfig c = self->collectConfig();

    if (c.name.empty()) {
        show_alert(GTK_WINDOW(self->dialog), "Please enter a VM name.");
        return;
    }

    std::string vm_dir = c.vm_dir;
    g_mkdir_with_parents(vm_dir.c_str(), 0755);
    g_mkdir_with_parents((vm_dir + "/snapshots").c_str(), 0755);
    g_mkdir_with_parents((vm_dir + "/logs").c_str(), 0755);

    StorageManager::createDisk(c.disk_path, c.disk_format, c.disk_size_gb);

    std::string cmd = CommandBuilder::buildCommand(c, Settings::get().qemu_bin_path);
    std::ofstream sh(vm_dir + "/start.sh");
    sh << "#!/bin/bash\n# Generated by QEMU Manager\n# VM: " << c.name << "\n\n" << cmd << "\n";
    sh.close();
    system(("chmod +x \"" + vm_dir + "/start.sh\"").c_str());

    if (self->on_create_cb) self->on_create_cb(c);
    gtk_window_destroy(GTK_WINDOW(self->dialog));
    delete self;
}

void VMWizard::onCancelClicked(GtkButton*, gpointer data) {
    VMWizard* self = (VMWizard*)data;
    gtk_window_destroy(GTK_WINDOW(self->dialog));
    delete self;
}

#include "mainwindow.h"
#include "vmwizard.h"
#include "commandbuilder.h"
#include "storagemanager.h"
#include "settings.h"
#include "templates.h"
#include <glib.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <sys/stat.h>
#include <array>
#include <memory>
#include <cstring>

GtkWidget* buildDashboardWidget(int running, int stopped);
GtkWidget* buildVMListWidget(const std::vector<VMConfig>& vms);
GtkWidget* buildLogViewer(const std::string& log_path);
GtkWidget* buildSerialConsole();
GtkWidget* buildDownloadManager();
bool saveVMConfig(const VMConfig& vm);
VMConfig loadVMConfig(const std::string& vm_dir);

static std::string exec_cmd2(const std::string& cmd) {
    std::array<char,256> buf; std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(),"r"),pclose);
    if (!pipe) return "";
    while (fgets(buf.data(), buf.size(), pipe.get()) != nullptr) result += buf.data();
    return result;
}

// GTK4 alert helper — replaces gtk_message_dialog + gtk_dialog_run
static void mw_show_alert(GtkWindow* parent, const std::string& msg) {
    GtkAlertDialog* dlg = gtk_alert_dialog_new("%s", msg.c_str());
    gtk_alert_dialog_show(dlg, parent);
    g_object_unref(dlg);
}

// GTK4 confirm dialog helper (async)
struct ConfirmData {
    std::function<void()> on_yes;
    GtkWindow* parent;
};

static void on_confirm_response(GObject* src, GAsyncResult* res, gpointer user_data) {
    ConfirmData* cd = (ConfirmData*)user_data;
    int btn = gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(src), res, nullptr);
    if (btn == 1 && cd->on_yes) cd->on_yes();
    delete cd;
}

static void mw_confirm(GtkWindow* parent, const std::string& msg, std::function<void()> on_yes) {
    const char* btns[] = {"Cancel", "Yes", nullptr};
    GtkAlertDialog* dlg = gtk_alert_dialog_new("%s", msg.c_str());
    gtk_alert_dialog_set_buttons(dlg, btns);
    gtk_alert_dialog_set_cancel_button(dlg, 0);
    gtk_alert_dialog_set_default_button(dlg, 1);
    ConfirmData* cd = new ConfirmData{on_yes, parent};
    gtk_alert_dialog_choose(dlg, parent, nullptr, on_confirm_response, cd);
    g_object_unref(dlg);
}

// GTK4 folder picker (async)
struct FolderData { std::function<void(std::string)> cb; };

static void on_folder_chosen(GObject* src, GAsyncResult* res, gpointer user_data) {
    FolderData* fd = (FolderData*)user_data;
    GFile* file = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(src), res, nullptr);
    if (file) {
        char* path = g_file_get_path(file);
        if (path && fd->cb) fd->cb(path);
        g_free(path);
        g_object_unref(file);
    }
    delete fd;
}

MainWindow::MainWindow(GtkApplication* app) {
    loadVMs();
    buildUI(app);
}

void MainWindow::loadVMs() {
    vms.clear();
    std::string vm_folder = Settings::get().vm_folder;
    DIR* dir = opendir(vm_folder.c_str());
    if (!dir) return;
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.') continue;
        std::string vm_dir = vm_folder + "/" + ent->d_name;
        std::string cfg = vm_dir + "/vm.json";
        struct stat st;
        if (stat(cfg.c_str(), &st) == 0) {
            VMConfig vm = loadVMConfig(vm_dir);
            std::string pid_file = vm_dir + "/vm.pid";
            if (stat(pid_file.c_str(), &st) == 0) {
                std::ifstream pf(pid_file);
                std::string pid_str; pf >> pid_str;
                if (!pid_str.empty()) {
                    std::string check = "kill -0 " + pid_str + " 2>/dev/null && echo running";
                    std::string r = exec_cmd2(check);
                    if (r.find("running") != std::string::npos) vm.status = VMStatus::Running;
                }
            }
            vms.push_back(vm);
        }
    }
    closedir(dir);
}

void MainWindow::saveVM(const VMConfig& vm) {
    g_mkdir_with_parents(vm.vm_dir.c_str(), 0755);
    saveVMConfig(vm);
}

void MainWindow::buildUI(GtkApplication* app) {
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "QEMU Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 1100, 700);

    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css,
        "textview { font-family: monospace; font-size: 12px; }"
    );
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    buildToolbar();
    gtk_box_append(GTK_BOX(main_box), toolbar);
    gtk_box_append(GTK_BOX(main_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget* content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_vexpand(content, TRUE);
    gtk_box_append(GTK_BOX(main_box), content);

    buildSidebar();
    gtk_box_append(GTK_BOX(content), sidebar);
    gtk_box_append(GTK_BOX(content), gtk_separator_new(GTK_ORIENTATION_VERTICAL));

    stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_widget_set_hexpand(stack, TRUE);
    gtk_widget_set_vexpand(stack, TRUE);
    gtk_box_append(GTK_BOX(content), stack);

    buildDashboardPage();
    buildVMListPage();
    buildStoragePage();
    buildISOPage();
    buildSnapshotPage();
    buildTemplatesPage();
    buildDownloadsPage();
    buildLogsPage();
    buildSettingsPage();
    buildAboutPage();
}

void MainWindow::buildToolbar() {
    toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(toolbar, 8);
    gtk_widget_set_margin_end(toolbar, 8);
    gtk_widget_set_margin_top(toolbar, 6);
    gtk_widget_set_margin_bottom(toolbar, 6);

    auto make_btn = [&](const char* label, GCallback cb) -> GtkWidget* {
        GtkWidget* b = gtk_button_new_with_label(label);
        g_signal_connect(b, "clicked", cb, this);
        gtk_box_append(GTK_BOX(toolbar), b);
        return b;
    };

    GtkWidget* new_btn = make_btn("+ New VM", G_CALLBACK(onNewVMClicked));
    gtk_widget_add_css_class(new_btn, "suggested-action");
    make_btn("Import", G_CALLBACK(onImportClicked));
    make_btn("Clone",  G_CALLBACK(onCloneClicked));
    gtk_box_append(GTK_BOX(toolbar), gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    make_btn("▶ Start",   G_CALLBACK(onStartClicked));
    make_btn("■ Stop",    G_CALLBACK(onStopClicked));
    make_btn("⏸ Pause",  G_CALLBACK(onPauseClicked));
    make_btn("↺ Restart", G_CALLBACK(onRestartClicked));
    gtk_box_append(GTK_BOX(toolbar), gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    GtkWidget* del_btn = make_btn("Delete", G_CALLBACK(onDeleteClicked));
    gtk_widget_add_css_class(del_btn, "destructive-action");

    GtkWidget* spacer = gtk_label_new("");
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(toolbar), spacer);

    search_entry = gtk_search_entry_new();
    gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(search_entry), "Search VMs...");
    g_signal_connect(search_entry, "search-changed", G_CALLBACK(onSearchChanged), this);
    gtk_box_append(GTK_BOX(toolbar), search_entry);
}

void MainWindow::buildSidebar() {
    sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(sidebar, 180, -1);

    GtkWidget* listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(listbox), GTK_SELECTION_SINGLE);
    gtk_widget_set_vexpand(listbox, TRUE);
    g_signal_connect(listbox, "row-activated", G_CALLBACK(onSidebarRowActivated), this);

    const char* items[] = {
        "Dashboard","Virtual Machines","Storage","ISOs",
        "Snapshots","Templates","Downloads","Logs",
        "Settings","About",nullptr
    };
    const char* pages[] = {
        "dashboard","vms","storage","isos",
        "snapshots","templates","downloads","logs",
        "settings","about"
    };

    for (int i = 0; items[i]; i++) {
        GtkWidget* row_lbl = gtk_label_new(items[i]);
        gtk_label_set_xalign(GTK_LABEL(row_lbl), 0.0f);
        gtk_widget_set_margin_start(row_lbl, 16);
        gtk_widget_set_margin_end(row_lbl, 16);
        gtk_widget_set_margin_top(row_lbl, 10);
        gtk_widget_set_margin_bottom(row_lbl, 10);
        GtkWidget* list_row = gtk_list_box_row_new();
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(list_row), row_lbl);
        g_object_set_data(G_OBJECT(list_row), "page", (gpointer)pages[i]);
        gtk_list_box_append(GTK_LIST_BOX(listbox), list_row);
    }

    gtk_box_append(GTK_BOX(sidebar), listbox);
    GtkListBoxRow* first = gtk_list_box_get_row_at_index(GTK_LIST_BOX(listbox), 0);
    gtk_list_box_select_row(GTK_LIST_BOX(listbox), first);
}

void MainWindow::buildDashboardPage() {
    int running = 0, stopped = 0;
    for (const auto& v : vms) {
        if (v.status == VMStatus::Running) running++; else stopped++;
    }
    GtkWidget* w = buildDashboardWidget(running, stopped);
    gtk_stack_add_named(GTK_STACK(stack), w, "dashboard");
}

void MainWindow::buildVMListPage() {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget* lbl = gtk_label_new("Virtual Machines");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
    gtk_label_set_attributes(GTK_LABEL(lbl), attrs);
    pango_attr_list_unref(attrs);
    gtk_widget_set_margin_start(lbl, 16);
    gtk_widget_set_margin_top(lbl, 12);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);
    gtk_box_append(GTK_BOX(box), lbl);
    gtk_box_append(GTK_BOX(box), buildVMListWidget(vms));
    gtk_stack_add_named(GTK_STACK(stack), box, "vms");
}

void MainWindow::buildStoragePage() {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_top(box, 16);
    GtkWidget* lbl = gtk_label_new("Storage Manager");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
    gtk_label_set_attributes(GTK_LABEL(lbl), attrs);
    pango_attr_list_unref(attrs);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);
    gtk_box_append(GTK_BOX(box), lbl);
    GtkWidget* info = gtk_label_new("Manage VM disk images — create, resize, convert and clone using qemu-img.");
    gtk_label_set_xalign(GTK_LABEL(info), 0);
    gtk_widget_add_css_class(info, "dim-label");
    gtk_box_append(GTK_BOX(box), info);
    GtkWidget* btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(btn_box, 8);
    for (const char* s : {"Create Image","Resize Image","Convert Image","Clone Image"})
        gtk_box_append(GTK_BOX(btn_box), gtk_button_new_with_label(s));
    gtk_box_append(GTK_BOX(box), btn_box);
    gtk_stack_add_named(GTK_STACK(stack), box, "storage");
}

void MainWindow::buildISOPage() {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_top(box, 16);
    GtkWidget* lbl = gtk_label_new("ISO Library");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
    gtk_label_set_attributes(GTK_LABEL(lbl), attrs);
    pango_attr_list_unref(attrs);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);
    gtk_box_append(GTK_BOX(box), lbl);
    auto isos = StorageManager::listISOs(Settings::get().iso_folder);
    if (isos.empty()) {
        GtkWidget* e = gtk_label_new(("No ISOs found in " + Settings::get().iso_folder).c_str());
        gtk_widget_add_css_class(e, "dim-label");
        gtk_label_set_xalign(GTK_LABEL(e), 0);
        gtk_box_append(GTK_BOX(box), e);
    } else {
        GtkWidget* lb = gtk_list_box_new();
        gtk_widget_add_css_class(lb, "boxed-list");
        for (const auto& iso : isos) {
            GtkWidget* r = gtk_label_new(iso.c_str());
            gtk_label_set_xalign(GTK_LABEL(r), 0);
            gtk_widget_set_margin_start(r, 12);
            gtk_widget_set_margin_top(r, 8);
            gtk_widget_set_margin_bottom(r, 8);
            gtk_list_box_append(GTK_LIST_BOX(lb), r);
        }
        gtk_box_append(GTK_BOX(box), lb);
    }
    gtk_stack_add_named(GTK_STACK(stack), box, "isos");
}

void MainWindow::buildSnapshotPage() {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_top(box, 16);
    GtkWidget* lbl = gtk_label_new("Snapshots");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
    gtk_label_set_attributes(GTK_LABEL(lbl), attrs);
    pango_attr_list_unref(attrs);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);
    gtk_box_append(GTK_BOX(box), lbl);
    GtkWidget* info = gtk_label_new("Select a VM from the list, then manage its snapshots here.");
    gtk_label_set_xalign(GTK_LABEL(info), 0);
    gtk_widget_add_css_class(info, "dim-label");
    gtk_box_append(GTK_BOX(box), info);
    GtkWidget* btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(btn_box, 8);
    gtk_box_append(GTK_BOX(btn_box), gtk_button_new_with_label("Create Snapshot"));
    gtk_box_append(GTK_BOX(btn_box), gtk_button_new_with_label("Restore Snapshot"));
    gtk_box_append(GTK_BOX(btn_box), gtk_button_new_with_label("Delete Snapshot"));
    gtk_box_append(GTK_BOX(box), btn_box);
    gtk_stack_add_named(GTK_STACK(stack), box, "snapshots");
}

void MainWindow::buildTemplatesPage() {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_end(box, 16);
    GtkWidget* lbl = gtk_label_new("VM Templates");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
    gtk_label_set_attributes(GTK_LABEL(lbl), attrs);
    pango_attr_list_unref(attrs);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);
    gtk_box_append(GTK_BOX(box), lbl);

    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    GtkWidget* listbox = gtk_list_box_new();
    gtk_widget_add_css_class(listbox, "boxed-list");

    for (const auto& t : getTemplates()) {
        GtkWidget* row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_margin_start(row_box, 12);
        gtk_widget_set_margin_end(row_box, 12);
        gtk_widget_set_margin_top(row_box, 8);
        gtk_widget_set_margin_bottom(row_box, 8);

        GtkWidget* col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        GtkWidget* name_lbl = gtk_label_new(t.name.c_str());
        gtk_label_set_xalign(GTK_LABEL(name_lbl), 0);
        PangoAttrList* ba = pango_attr_list_new();
        pango_attr_list_insert(ba, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
        gtk_label_set_attributes(GTK_LABEL(name_lbl), ba);
        pango_attr_list_unref(ba);

        std::string sub = std::to_string(t.cores) + " CPU, " +
            std::to_string(t.ram_mb) + "MB RAM, " +
            std::to_string(t.disk_size_gb) + "GB disk";
        GtkWidget* sub_lbl = gtk_label_new(sub.c_str());
        gtk_label_set_xalign(GTK_LABEL(sub_lbl), 0);
        gtk_widget_add_css_class(sub_lbl, "dim-label");

        gtk_box_append(GTK_BOX(col), name_lbl);
        gtk_box_append(GTK_BOX(col), sub_lbl);
        gtk_widget_set_hexpand(col, TRUE);

        GtkWidget* use_btn = gtk_button_new_with_label("Use Template");
        struct TD { VMTemplate t; MainWindow* win; };
        TD* td = new TD{t, this};
        g_signal_connect(use_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer p) {
            TD* d = (TD*)p;
            std::string vm_dir = Settings::get().vm_folder + "/" + d->t.name;
            VMWizard* wiz = new VMWizard(GTK_WINDOW(d->win->window),
                [d](VMConfig vm) {
                    d->win->vms.push_back(vm);
                    d->win->saveVM(vm);
                    delete d;
                });
            wiz->show();
        }), td);

        gtk_box_append(GTK_BOX(row_box), col);
        gtk_box_append(GTK_BOX(row_box), use_btn);
        gtk_list_box_append(GTK_LIST_BOX(listbox), row_box);
    }

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), listbox);
    gtk_box_append(GTK_BOX(box), scroll);
    gtk_stack_add_named(GTK_STACK(stack), box, "templates");
}

void MainWindow::buildDownloadsPage() {
    gtk_stack_add_named(GTK_STACK(stack), buildDownloadManager(), "downloads");
}

void MainWindow::buildLogsPage() {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_end(box, 16);
    GtkWidget* lbl = gtk_label_new("Logs");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
    gtk_label_set_attributes(GTK_LABEL(lbl), attrs);
    pango_attr_list_unref(attrs);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);
    gtk_box_append(GTK_BOX(box), lbl);
    std::string log_path = selected_vm_name.empty() ? "" :
        Settings::get().vm_folder + "/" + selected_vm_name + "/logs/qemu.log";
    gtk_box_append(GTK_BOX(box), buildLogViewer(log_path));
    gtk_stack_add_named(GTK_STACK(stack), box, "logs");
}

void MainWindow::buildSettingsPage() {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_widget_set_margin_start(grid, 24);
    gtk_widget_set_margin_end(grid, 24);
    gtk_widget_set_margin_top(grid, 20);

    int row = 0;
    auto& s = Settings::get();

    GtkWidget* title = gtk_label_new("Settings");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
    gtk_label_set_attributes(GTK_LABEL(title), attrs);
    pango_attr_list_unref(attrs);
    gtk_label_set_xalign(GTK_LABEL(title), 0);
    gtk_grid_attach(GTK_GRID(grid), title, 0, row++, 2, 1);

    auto add_lbl = [&](const char* t, int r) {
        GtkWidget* l = gtk_label_new(t);
        gtk_label_set_xalign(GTK_LABEL(l), 1.0f);
        gtk_grid_attach(GTK_GRID(grid), l, 0, r, 1, 1);
    };

    add_lbl("QEMU Binary Dir:", row);
    GtkWidget* qemu_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(qemu_entry), s.qemu_bin_path.c_str());
    gtk_entry_set_placeholder_text(GTK_ENTRY(qemu_entry), "/usr/bin (leave empty for auto)");
    gtk_widget_set_hexpand(qemu_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), qemu_entry, 1, row++, 1, 1);

    add_lbl("VM Folder:", row);
    GtkWidget* vm_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(vm_entry), s.vm_folder.c_str());
    gtk_widget_set_hexpand(vm_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), vm_entry, 1, row++, 1, 1);

    add_lbl("ISO Folder:", row);
    GtkWidget* iso_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(iso_entry), s.iso_folder.c_str());
    gtk_widget_set_hexpand(iso_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), iso_entry, 1, row++, 1, 1);

    add_lbl("Default Extra Args:", row);
    GtkWidget* args_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(args_entry), s.default_args.c_str());
    gtk_entry_set_placeholder_text(GTK_ENTRY(args_entry), "-accel tcg,thread=multi");
    gtk_widget_set_hexpand(args_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), args_entry, 1, row++, 1, 1);

    GtkWidget* save_btn = gtk_button_new_with_label("Save Settings");
    gtk_widget_add_css_class(save_btn, "suggested-action");

    struct SD { GtkWidget *qe, *ve, *ie, *ae; GtkWindow* win; };
    SD* sd = new SD{qemu_entry, vm_entry, iso_entry, args_entry, GTK_WINDOW(window)};
    g_signal_connect(save_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer p) {
        SD* d = (SD*)p;
        auto& s = Settings::get();
        s.qemu_bin_path = gtk_editable_get_text(GTK_EDITABLE(d->qe));
        s.vm_folder     = gtk_editable_get_text(GTK_EDITABLE(d->ve));
        s.iso_folder    = gtk_editable_get_text(GTK_EDITABLE(d->ie));
        s.default_args  = gtk_editable_get_text(GTK_EDITABLE(d->ae));
        std::string home = g_get_home_dir();
        Settings::save(home + "/.config/qemu-manager/settings.json");
        GtkAlertDialog* dlg = gtk_alert_dialog_new("Settings saved.");
        gtk_alert_dialog_show(dlg, d->win);
        g_object_unref(dlg);
    }), sd);

    gtk_grid_attach(GTK_GRID(grid), save_btn, 1, row++, 1, 1);
    gtk_stack_add_named(GTK_STACK(stack), grid, "settings");
}

void MainWindow::buildAboutPage() {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);

    GtkWidget* title = gtk_label_new("QEMU Manager");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_scale_new(2.0));
    gtk_label_set_attributes(GTK_LABEL(title), attrs);
    pango_attr_list_unref(attrs);

    GtkWidget* ver  = gtk_label_new("Version 1.0.0");
    GtkWidget* desc = gtk_label_new(
        "A native QEMU virtual machine manager.\n"
        "No root required. No libvirt. No systemd.\n"
        "Works in Termux + PRoot and regular Linux.\n\n"
        "Architectures: x86_64, i386, aarch64, arm,\n"
        "riscv64, riscv32, mips, mipsel, ppc64, sparc");
    gtk_label_set_justify(GTK_LABEL(desc), GTK_JUSTIFY_CENTER);
    gtk_widget_add_css_class(ver, "dim-label");
    gtk_widget_add_css_class(desc, "dim-label");
    GtkWidget* lic = gtk_label_new("Licensed under GPL v2");
    gtk_widget_add_css_class(lic, "dim-label");

    gtk_box_append(GTK_BOX(box), title);
    gtk_box_append(GTK_BOX(box), ver);
    gtk_box_append(GTK_BOX(box), desc);
    gtk_box_append(GTK_BOX(box), lic);
    gtk_stack_add_named(GTK_STACK(stack), box, "about");
}

void MainWindow::onSidebarRowActivated(GtkListBox*, GtkListBoxRow* row, gpointer data) {
    MainWindow* self = (MainWindow*)data;
    const char* page = (const char*)g_object_get_data(G_OBJECT(row), "page");
    if (page) gtk_stack_set_visible_child_name(GTK_STACK(self->stack), page);
}

void MainWindow::onSearchChanged(GtkSearchEntry*, gpointer) {}

void MainWindow::onNewVM() {
    VMWizard* wizard = new VMWizard(GTK_WINDOW(window), [this](VMConfig vm) {
        vms.push_back(vm);
        saveVM(vm);
    });
    wizard->show();
}

void MainWindow::onStartVM() {
    if (selected_vm_name.empty()) return;
    for (auto& vm : vms) {
        if (vm.name == selected_vm_name) {
            std::string log_dir = vm.vm_dir + "/logs";
            g_mkdir_with_parents(log_dir.c_str(), 0755);
            std::string cmd = CommandBuilder::buildCommand(vm, Settings::get().qemu_bin_path);
            cmd += " > \"" + log_dir + "/qemu.log\" 2>&1 &";
            system(cmd.c_str());
            vm.status = VMStatus::Running;
            time_t t = time(nullptr);
            char buf[64];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&t));
            vm.last_started = buf;
            saveVM(vm);
            break;
        }
    }
}

void MainWindow::onStopVM() {
    if (selected_vm_name.empty()) return;
    for (auto& vm : vms) {
        if (vm.name == selected_vm_name) {
            std::string pid_file = vm.vm_dir + "/vm.pid";
            std::ifstream pf(pid_file);
            std::string pid; pf >> pid;
            if (!pid.empty()) system(("kill " + pid).c_str());
            vm.status = VMStatus::Stopped;
            saveVM(vm);
            break;
        }
    }
}

void MainWindow::onPauseVM() {
    if (selected_vm_name.empty()) return;
    for (auto& vm : vms) {
        if (vm.name == selected_vm_name && vm.status == VMStatus::Running) {
            std::string cmd = "echo 'stop' | socat - UNIX-CONNECT:\"" +
                vm.vm_dir + "/monitor.sock\" 2>/dev/null";
            system(cmd.c_str());
            vm.status = VMStatus::Paused;
            break;
        }
    }
}

void MainWindow::onRestartVM() { onStopVM(); onStartVM(); }

void MainWindow::onDeleteVM() {
    if (selected_vm_name.empty()) return;
    std::string name = selected_vm_name;
    mw_confirm(GTK_WINDOW(window),
        "Delete VM '" + name + "'?\nThis will remove all files permanently.",
        [this, name]() {
            for (auto it = vms.begin(); it != vms.end(); ++it) {
                if (it->name == name) {
                    std::string cmd = "rm -rf \"" + it->vm_dir + "\"";
                    system(cmd.c_str());
                    vms.erase(it);
                    break;
                }
            }
            selected_vm_name = "";
        });
}

void MainWindow::onCloneVM() {
    if (selected_vm_name.empty()) return;
    for (const auto& vm : vms) {
        if (vm.name == selected_vm_name) {
            VMConfig clone = vm;
            clone.name = vm.name + "-clone";
            clone.vm_dir = Settings::get().vm_folder + "/" + clone.name;
            g_mkdir_with_parents(clone.vm_dir.c_str(), 0755);
            StorageManager::cloneDisk(vm.disk_path, clone.vm_dir + "/disk.qcow2");
            clone.disk_path = clone.vm_dir + "/disk.qcow2";
            saveVM(clone);
            vms.push_back(clone);
            break;
        }
    }
}

void MainWindow::onImportVM() {
    GtkFileDialog* dlg = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dlg, "Import VM Directory");
    FolderData* fd = new FolderData{[this](std::string path) {
        VMConfig vm = loadVMConfig(path);
        if (!vm.name.empty()) vms.push_back(vm);
    }};
    gtk_file_dialog_select_folder(dlg, GTK_WINDOW(window), nullptr, on_folder_chosen, fd);
    g_object_unref(dlg);
}

void MainWindow::onNewVMClicked(GtkButton*, gpointer d)     { ((MainWindow*)d)->onNewVM(); }
void MainWindow::onStartClicked(GtkButton*, gpointer d)     { ((MainWindow*)d)->onStartVM(); }
void MainWindow::onStopClicked(GtkButton*, gpointer d)      { ((MainWindow*)d)->onStopVM(); }
void MainWindow::onPauseClicked(GtkButton*, gpointer d)     { ((MainWindow*)d)->onPauseVM(); }
void MainWindow::onRestartClicked(GtkButton*, gpointer d)   { ((MainWindow*)d)->onRestartVM(); }
void MainWindow::onDeleteClicked(GtkButton*, gpointer d)    { ((MainWindow*)d)->onDeleteVM(); }
void MainWindow::onCloneClicked(GtkButton*, gpointer d)     { ((MainWindow*)d)->onCloneVM(); }
void MainWindow::onImportClicked(GtkButton*, gpointer d)    { ((MainWindow*)d)->onImportVM(); }

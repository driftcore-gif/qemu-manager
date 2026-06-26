#include <gtk/gtk.h>
#include "vm.h"
#include <string>
#include <vector>

static std::string statusStr(VMStatus s) {
    switch(s) {
        case VMStatus::Running:   return "● Running";
        case VMStatus::Stopped:   return "○ Stopped";
        case VMStatus::Paused:    return "⏸ Paused";
        case VMStatus::Suspended: return "💤 Suspended";
        default: return "Unknown";
    }
}

static std::string archStr(VMArch a) {
    switch(a) {
        case VMArch::x86_64:  return "x86_64";
        case VMArch::i386:    return "i386";
        case VMArch::aarch64: return "aarch64";
        case VMArch::arm:     return "arm";
        case VMArch::riscv64: return "riscv64";
        case VMArch::riscv32: return "riscv32";
        case VMArch::mips:    return "mips";
        case VMArch::mipsel:  return "mipsel";
        case VMArch::ppc64:   return "ppc64";
        case VMArch::sparc:   return "sparc";
        default: return "x86_64";
    }
}

GtkWidget* buildVMRow(const VMConfig& vm) {
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(row, 12);
    gtk_widget_set_margin_end(row, 12);
    gtk_widget_set_margin_top(row, 8);
    gtk_widget_set_margin_bottom(row, 8);

    // Icon/name column
    GtkWidget* left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget* name_lbl = gtk_label_new(vm.name.c_str());
    gtk_label_set_xalign(GTK_LABEL(name_lbl), 0);
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(name_lbl), attrs);
    pango_attr_list_unref(attrs);

    std::string desc = archStr(vm.arch) + " | " + std::to_string(vm.cores) + " CPU | " +
        std::to_string(vm.ram_mb) + " MB | " + std::to_string(vm.disk_size_gb) + " GB";
    GtkWidget* desc_lbl = gtk_label_new(desc.c_str());
    gtk_label_set_xalign(GTK_LABEL(desc_lbl), 0);
    gtk_widget_add_css_class(desc_lbl, "dim-label");

    gtk_box_append(GTK_BOX(left), name_lbl);
    gtk_box_append(GTK_BOX(left), desc_lbl);
    gtk_widget_set_hexpand(left, TRUE);

    // Status
    GtkWidget* status_lbl = gtk_label_new(statusStr(vm.status).c_str());
    if (vm.status == VMStatus::Running)
        gtk_widget_add_css_class(status_lbl, "success");

    // Last started
    std::string ls = vm.last_started.empty() ? "Never" : vm.last_started;
    GtkWidget* last_lbl = gtk_label_new(("Last: " + ls).c_str());
    gtk_widget_add_css_class(last_lbl, "dim-label");

    gtk_box_append(GTK_BOX(row), left);
    gtk_box_append(GTK_BOX(row), status_lbl);
    gtk_box_append(GTK_BOX(row), last_lbl);

    return row;
}

GtkWidget* buildVMListWidget(const std::vector<VMConfig>& vms) {
    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget* listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(listbox), GTK_SELECTION_SINGLE);
    gtk_widget_add_css_class(listbox, "boxed-list");

    if (vms.empty()) {
        GtkWidget* empty = gtk_label_new("No virtual machines yet.\nClick 'New VM' to create one.");
        gtk_label_set_justify(GTK_LABEL(empty), GTK_JUSTIFY_CENTER);
        gtk_widget_set_margin_top(empty, 40);
        gtk_widget_add_css_class(empty, "dim-label");
        gtk_list_box_append(GTK_LIST_BOX(listbox), empty);
    } else {
        for (const auto& vm : vms) {
            GtkWidget* row_widget = buildVMRow(vm);
            gtk_list_box_append(GTK_LIST_BOX(listbox), row_widget);
        }
    }

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), listbox);
    gtk_widget_set_vexpand(scroll, TRUE);
    return scroll;
}

#include <gtk/gtk.h>
#include <string>
#include <vector>

struct DownloadEntry {
    std::string name;
    std::string url;
    std::string description;
    std::string category; // iso, firmware, driver
};

static std::vector<DownloadEntry> getDownloadList() {
    return {
        {"Ubuntu 24.04 LTS", "https://releases.ubuntu.com/24.04/ubuntu-24.04-desktop-amd64.iso", "Ubuntu Desktop x86_64", "iso"},
        {"Debian 12 Netinst", "https://cdimage.debian.org/debian-cd/current/amd64/iso-cd/", "Debian minimal install", "iso"},
        {"Alpine Linux", "https://dl-cdn.alpinelinux.org/alpine/latest-stable/releases/x86_64/", "Lightweight Alpine x86_64", "iso"},
        {"OVMF UEFI Firmware", "https://github.com/tianocore/edk2/releases", "UEFI firmware for x86_64", "firmware"},
        {"VirtIO Drivers (Win)", "https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/", "VirtIO drivers for Windows guests", "driver"},
    };
}

GtkWidget* buildDownloadManager() {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 12);

    GtkWidget* lbl = gtk_label_new("Downloads — ISOs, Firmware, Drivers");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.2));
    gtk_label_set_attributes(GTK_LABEL(lbl), attrs);
    pango_attr_list_unref(attrs);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);

    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    GtkWidget* listbox = gtk_list_box_new();
    gtk_widget_add_css_class(listbox, "boxed-list");

    for (const auto& d : getDownloadList()) {
        GtkWidget* row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_margin_start(row_box, 12);
        gtk_widget_set_margin_end(row_box, 12);
        gtk_widget_set_margin_top(row_box, 8);
        gtk_widget_set_margin_bottom(row_box, 8);

        GtkWidget* col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        GtkWidget* name_lbl = gtk_label_new(d.name.c_str());
        gtk_label_set_xalign(GTK_LABEL(name_lbl), 0);
        PangoAttrList* ba = pango_attr_list_new();
        pango_attr_list_insert(ba, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
        gtk_label_set_attributes(GTK_LABEL(name_lbl), ba);
        pango_attr_list_unref(ba);

        GtkWidget* desc_lbl = gtk_label_new(d.description.c_str());
        gtk_label_set_xalign(GTK_LABEL(desc_lbl), 0);
        gtk_widget_add_css_class(desc_lbl, "dim-label");

        gtk_box_append(GTK_BOX(col), name_lbl);
        gtk_box_append(GTK_BOX(col), desc_lbl);
        gtk_widget_set_hexpand(col, TRUE);

        GtkWidget* badge = gtk_label_new(d.category.c_str());
        gtk_widget_add_css_class(badge, "dim-label");

        GtkWidget* dl_btn = gtk_button_new_with_label("Open URL");
        std::string* url_ptr = new std::string(d.url);
        g_signal_connect(dl_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer p) {
            std::string* url = (std::string*)p;
            GError* err = nullptr;
            g_app_info_launch_default_for_uri(url->c_str(), nullptr, &err);
            if (err) g_error_free(err);
        }), url_ptr);

        gtk_box_append(GTK_BOX(row_box), col);
        gtk_box_append(GTK_BOX(row_box), badge);
        gtk_box_append(GTK_BOX(row_box), dl_btn);
        gtk_list_box_append(GTK_LIST_BOX(listbox), row_box);
    }

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), listbox);
    gtk_box_append(GTK_BOX(box), lbl);
    gtk_box_append(GTK_BOX(box), scroll);
    return box;
}

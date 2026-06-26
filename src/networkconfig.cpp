#include <gtk/gtk.h>
#include "vm.h"

GtkWidget* buildNetworkConfigWidget(VMConfig& vm) {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 12);

    GtkWidget* lbl = gtk_label_new("Network Configuration");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(lbl), attrs);
    pango_attr_list_unref(attrs);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);

    GtkWidget* mode_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* mode_lbl = gtk_label_new("Network Mode:");
    GtkWidget* mode_combo = gtk_drop_down_new_from_strings(
        (const char*[]){"User (NAT)", "TAP", "Bridge", "Socket", nullptr});
    gtk_drop_down_set_selected(GTK_DROP_DOWN(mode_combo), (int)vm.net_mode);

    gtk_box_append(GTK_BOX(mode_box), mode_lbl);
    gtk_box_append(GTK_BOX(mode_box), mode_combo);

    GtkWidget* info = gtk_label_new(
        "User mode: No root needed. TAP/Bridge: needs kernel support.\n"
        "Port forwarding available in User mode.");
    gtk_widget_add_css_class(info, "dim-label");
    gtk_label_set_wrap(GTK_LABEL(info), TRUE);
    gtk_label_set_xalign(GTK_LABEL(info), 0);

    gtk_box_append(GTK_BOX(box), lbl);
    gtk_box_append(GTK_BOX(box), mode_box);
    gtk_box_append(GTK_BOX(box), info);
    return box;
}

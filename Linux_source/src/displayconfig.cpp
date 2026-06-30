#include <gtk/gtk.h>
#include "vm.h"

GtkWidget* buildDisplayConfigWidget(VMConfig& vm) {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 12);

    GtkWidget* lbl = gtk_label_new("Display Configuration");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(lbl), attrs);
    pango_attr_list_unref(attrs);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);

    GtkWidget* disp_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* disp_lbl = gtk_label_new("Display Backend:");
    static const char* disp_names[] = {"GTK", "SDL", "SPICE", "VNC", "EGL", "Headless", nullptr};
    GtkWidget* disp_combo = gtk_drop_down_new_from_strings(disp_names);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(disp_combo), (int)vm.display);

    gtk_box_append(GTK_BOX(disp_box), disp_lbl);
    gtk_box_append(GTK_BOX(disp_box), disp_combo);

    GtkWidget* gpu_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* gpu_lbl = gtk_label_new("GPU Device:");
    static const char* gpu_names[] = {"VGA", "VirtIO GPU", "QXL", "Cirrus", "VMware SVGA", "None", nullptr};
    GtkWidget* gpu_combo = gtk_drop_down_new_from_strings(gpu_names);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(gpu_combo), (int)vm.gpu);

    gtk_box_append(GTK_BOX(gpu_box), gpu_lbl);
    gtk_box_append(GTK_BOX(gpu_box), gpu_combo);

    gtk_box_append(GTK_BOX(box), lbl);
    gtk_box_append(GTK_BOX(box), disp_box);
    gtk_box_append(GTK_BOX(box), gpu_box);
    return box;
}

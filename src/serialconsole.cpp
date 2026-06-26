#include <gtk/gtk.h>
#include <string>

GtkWidget* buildSerialConsole() {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 12);

    GtkWidget* lbl = gtk_label_new("Serial Console");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.2));
    gtk_label_set_attributes(GTK_LABEL(lbl), attrs);
    pango_attr_list_unref(attrs);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);

    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);

    GtkWidget* view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
    GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    gtk_text_buffer_set_text(buf,
        "Serial console output will appear here when a VM is running with -serial stdio.\n"
        "Use QEMU monitor for advanced control.\n", -1);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), view);

    GtkWidget* input_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* input_entry = gtk_entry_new();
    gtk_widget_set_hexpand(input_entry, TRUE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(input_entry), "Type command and press Enter...");
    GtkWidget* send_btn = gtk_button_new_with_label("Send");

    gtk_box_append(GTK_BOX(input_box), input_entry);
    gtk_box_append(GTK_BOX(input_box), send_btn);

    gtk_box_append(GTK_BOX(box), lbl);
    gtk_box_append(GTK_BOX(box), scroll);
    gtk_box_append(GTK_BOX(box), input_box);
    return box;
}

#include <gtk/gtk.h>
#include <string>
#include <fstream>
#include <sstream>

GtkWidget* buildLogViewer(const std::string& log_path) {
    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_widget_set_hexpand(scroll, TRUE);

    GtkWidget* view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD_CHAR);

    GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));

    std::string content;
    if (!log_path.empty()) {
        std::ifstream f(log_path);
        if (f.is_open()) {
            content = std::string((std::istreambuf_iterator<char>(f)),
                std::istreambuf_iterator<char>());
        }
    }
    if (content.empty()) content = "[No logs yet. Start a VM to see output here.]\n";
    gtk_text_buffer_set_text(buf, content.c_str(), -1);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), view);
    return scroll;
}

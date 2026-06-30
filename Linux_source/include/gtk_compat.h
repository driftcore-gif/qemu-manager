#pragma once
#include <gtk/gtk.h>
#include <string>
#include <functional>

#if !GTK_CHECK_VERSION(4, 10, 0)

struct CompatFileData {
    GtkEntry* target;
};

static void compat_file_open_response(GtkDialog* dlg, int response, gpointer ud) {
    CompatFileData* fd = (CompatFileData*)ud;
    if (response == GTK_RESPONSE_ACCEPT) {
        GFile* file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dlg));
        if (file) {
            char* path = g_file_get_path(file);
            if (path) { gtk_editable_set_text(GTK_EDITABLE(fd->target), path); g_free(path); }
            g_object_unref(file);
        }
    }
    delete fd;
    gtk_window_destroy(GTK_WINDOW(dlg));
}

static inline void compat_open_file_chooser(GtkWindow* parent,
                                             const char* title,
                                             GtkEntry*  target_entry,
                                             const char* pattern = "*.iso",
                                             const char* filter_name = "ISO Images")
{
    GtkWidget* dlg = gtk_file_chooser_dialog_new(title, parent,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open",   GTK_RESPONSE_ACCEPT, nullptr);
    if (pattern) {
        GtkFileFilter* f = gtk_file_filter_new();
        gtk_file_filter_add_pattern(f, pattern);
        gtk_file_filter_set_name(f, filter_name);
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), f);
    }
    CompatFileData* fd = new CompatFileData{target_entry};
    g_signal_connect(dlg, "response", G_CALLBACK(compat_file_open_response), fd);
    gtk_widget_show(dlg);
}

struct CompatFolderData {
    std::function<void(std::string)> cb;
};

static void compat_folder_response(GtkDialog* dlg, int response, gpointer ud) {
    CompatFolderData* fd = (CompatFolderData*)ud;
    if (response == GTK_RESPONSE_ACCEPT) {
        GFile* file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dlg));
        if (file) {
            char* path = g_file_get_path(file);
            if (path && fd->cb) fd->cb(path);
            g_free(path);
            g_object_unref(file);
        }
    }
    delete fd;
    gtk_window_destroy(GTK_WINDOW(dlg));
}

static inline void compat_select_folder_chooser(GtkWindow* parent,
                                                  const char* title,
                                                  std::function<void(std::string)> callback)
{
    GtkWidget* dlg = gtk_file_chooser_dialog_new(title, parent,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Select", GTK_RESPONSE_ACCEPT, nullptr);
    CompatFolderData* fd = new CompatFolderData{callback};
    g_signal_connect(dlg, "response", G_CALLBACK(compat_folder_response), fd);
    gtk_widget_show(dlg);
}

static inline void compat_show_alert(GtkWindow* parent, const char* message)
{
    GtkWidget* dlg = gtk_message_dialog_new(parent,
        GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", message);
    g_signal_connect(dlg, "response", G_CALLBACK(gtk_window_destroy), nullptr);
    gtk_widget_show(dlg);
}

struct CompatConfirmData {
    std::function<void()> on_yes;
};

static void compat_confirm_response(GtkDialog* dlg, int response, gpointer ud) {
    CompatConfirmData* cd = (CompatConfirmData*)ud;
    if (response == GTK_RESPONSE_YES && cd->on_yes) cd->on_yes();
    delete cd;
    gtk_window_destroy(GTK_WINDOW(dlg));
}

static inline void compat_confirm_dialog(GtkWindow* parent,
                                          const char* message,
                                          std::function<void()> on_yes)
{
    GtkWidget* dlg = gtk_message_dialog_new(parent,
        GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", message);
    CompatConfirmData* cd = new CompatConfirmData{on_yes};
    g_signal_connect(dlg, "response", G_CALLBACK(compat_confirm_response), cd);
    gtk_widget_show(dlg);
}

#endif // !GTK_CHECK_VERSION(4, 10, 0)

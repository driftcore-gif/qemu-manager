#include "app.h"
#include "mainwindow.h"
#include "settings.h"
#include <glib.h>
#include <string>

QEMUManagerApp::QEMUManagerApp() {
    app = gtk_application_new("com.qemumanager.app", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(onActivate), this);
}

int QEMUManagerApp::run(int argc, char* argv[]) {
    // Load settings
    std::string home = g_get_home_dir();
    std::string config_dir = home + "/.config/qemu-manager";
    g_mkdir_with_parents(config_dir.c_str(), 0755);
    Settings::load(config_dir + "/settings.json");

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

void QEMUManagerApp::onActivate(GtkApplication* gapp, gpointer user_data) {
    (void)user_data;
    MainWindow* win = new MainWindow(gapp);
    gtk_window_present(GTK_WINDOW(win->getWidget()));
}

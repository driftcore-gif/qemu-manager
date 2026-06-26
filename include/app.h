#pragma once
#include <gtk/gtk.h>

class QEMUManagerApp {
public:
    QEMUManagerApp();
    int run(int argc, char* argv[]);
private:
    GtkApplication* app;
    static void onActivate(GtkApplication* app, gpointer user_data);
};

#pragma once
#include <gtk/gtk.h>
#include "vm.h"
#include <vector>
#include <memory>
#include <string>

class MainWindow {
public:
    MainWindow(GtkApplication* app);
    GtkWidget* getWidget() { return window; }

private:
    GtkWidget* window;
    GtkWidget* sidebar;
    GtkWidget* stack;
    GtkWidget* toolbar;
    GtkWidget* search_entry;
    std::vector<VMConfig> vms;
    std::string selected_vm_name;

    void buildUI(GtkApplication* app);
    void buildSidebar();
    void buildToolbar();
    void buildDashboardPage();
    void buildVMListPage();
    void buildStoragePage();
    void buildISOPage();
    void buildSnapshotPage();
    void buildTemplatesPage();
    void buildDownloadsPage();
    void buildLogsPage();
    void buildSettingsPage();
    void buildAboutPage();

    void loadVMs();
    void saveVM(const VMConfig& vm);
    void refreshVMList();
    void updateDashboard();

    // Actions
    void onNewVM();
    void onStartVM();
    void onStopVM();
    void onPauseVM();
    void onRestartVM();
    void onDeleteVM();
    void onCloneVM();
    void onImportVM();

    // Static GTK callbacks
    static void onNewVMClicked(GtkButton*, gpointer data);
    static void onStartClicked(GtkButton*, gpointer data);
    static void onStopClicked(GtkButton*, gpointer data);
    static void onPauseClicked(GtkButton*, gpointer data);
    static void onRestartClicked(GtkButton*, gpointer data);
    static void onDeleteClicked(GtkButton*, gpointer data);
    static void onCloneClicked(GtkButton*, gpointer data);
    static void onImportClicked(GtkButton*, gpointer data);
    static void onSidebarRowActivated(GtkListBox*, GtkListBoxRow*, gpointer data);
    static void onSearchChanged(GtkSearchEntry*, gpointer data);
};

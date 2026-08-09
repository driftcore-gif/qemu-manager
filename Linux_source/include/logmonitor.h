#pragma once
#include <gtk/gtk.h>
#include <string>

// LogMonitor — a lightweight, non-modal window that tails a VM's log file
// live and shows a plain hint banner if the QEMU binary appears to be
// missing (no filesystem detection is ever performed up-front; this is
// purely reactive to what the process actually printed or its exit code).
class LogMonitor {
public:
    LogMonitor(GtkWindow* parent, const std::string& vm_name,
               const std::string& log_file, const std::string& pid_file,
               const std::string& expected_binary);
    ~LogMonitor();

    void show();

private:
    GtkWidget*  window        = nullptr;
    GtkWidget*  banner        = nullptr;
    GtkWidget*  banner_label  = nullptr;
    GtkWidget*  text_view     = nullptr;
    GtkTextBuffer* buffer     = nullptr;

    std::string log_file;
    std::string pid_file;
    std::string vm_name;
    std::string expected_binary;

    long        read_offset   = 0;
    guint       poll_source   = 0;
    int         poll_ticks    = 0;   // counts polls, used to give the process a moment to start
    bool        banner_shown  = false;

    void pollOnce();
    void appendText(const std::string& text);
    void showMissingBinaryBanner();
    bool processStillAlive();

    static gboolean onPollTimeout(gpointer user_data);
    static void     onWindowDestroy(GtkWidget*, gpointer user_data);
};

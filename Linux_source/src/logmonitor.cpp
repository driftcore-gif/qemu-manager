#include "logmonitor.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>

LogMonitor::LogMonitor(GtkWindow* parent, const std::string& name,
                       const std::string& logf, const std::string& pidf,
                       const std::string& bin)
    : log_file(logf), pid_file(pidf), vm_name(name), expected_binary(bin) {

    window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), ("Run Log — " + vm_name).c_str());
    gtk_window_set_default_size(GTK_WINDOW(window), 760, 480);
    if (parent) gtk_window_set_transient_for(GTK_WINDOW(window), parent);
    gtk_window_set_modal(GTK_WINDOW(window), FALSE);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    // Hint banner — hidden until we detect a problem
    banner = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(banner, 10); gtk_widget_set_margin_end(banner, 10);
    gtk_widget_set_margin_top(banner, 8);
    gtk_widget_add_css_class(banner, "error");
    banner_label = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(banner_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(banner_label), 0);
    gtk_widget_set_hexpand(banner_label, TRUE);
    gtk_box_append(GTK_BOX(banner), banner_label);
    gtk_widget_set_visible(banner, FALSE);
    gtk_box_append(GTK_BOX(vbox), banner);

    // Scrolled text view for live output
    GtkWidget* scroller = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroller, TRUE);
    gtk_widget_set_margin_start(scroller, 10); gtk_widget_set_margin_end(scroller, 10);
    gtk_widget_set_margin_top(scroller, 8);    gtk_widget_set_margin_bottom(scroller, 10);

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(text_view), TRUE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), text_view);
    gtk_box_append(GTK_BOX(vbox), scroller);

    // Bottom bar
    GtkWidget* btnbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(btnbar, 10); gtk_widget_set_margin_end(btnbar, 10);
    gtk_widget_set_margin_bottom(btnbar, 10);
    GtkWidget* close_btn = gtk_button_new_with_label("Close");
    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(gtk_window_destroy), window);
    gtk_box_append(GTK_BOX(btnbar), close_btn);
    gtk_box_append(GTK_BOX(vbox), btnbar);

    g_signal_connect(window, "destroy", G_CALLBACK(onWindowDestroy), this);
}

LogMonitor::~LogMonitor() {
    if (poll_source) g_source_remove(poll_source);
}

void LogMonitor::show() {
    gtk_window_present(GTK_WINDOW(window));
    // Poll every 400ms — plain file tail + PID liveness check
    poll_source = g_timeout_add(400, onPollTimeout, this);
    pollOnce();
}

bool LogMonitor::processStillAlive() {
    std::ifstream pf(pid_file);
    if (!pf) return false;
    pid_t pid = 0;
    pf >> pid;
    if (pid <= 0) return false;
    return kill(pid, 0) == 0;
}

void LogMonitor::appendText(const std::string& text) {
    if (text.empty()) return;
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, text.c_str(), -1);
    // Auto-scroll to bottom
    GtkTextMark* mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(text_view), mark);
}

void LogMonitor::showMissingBinaryBanner() {
    if (banner_shown) return;
    banner_shown = true;
    std::string msg = "QEMU binary '" + expected_binary + "' was not found or failed to start.\n"
        "Install QEMU, then try again:\n"
        "  • Debian/Ubuntu:  sudo apt install qemu-system\n"
        "  • Termux:         pkg install qemu-system-x86-64  (or the arch you need)\n"
        "  • Fedora:         sudo dnf install qemu";
    gtk_label_set_text(GTK_LABEL(banner_label), msg.c_str());
    gtk_widget_set_visible(banner, TRUE);
}

void LogMonitor::pollOnce() {
    // Read any new bytes appended to the log file since last poll
    std::ifstream f(log_file, std::ios::binary);
    if (f) {
        f.seekg(0, std::ios::end);
        long size = (long)f.tellg();
        if (size > read_offset) {
            f.seekg(read_offset);
            std::ostringstream ss;
            ss << f.rdbuf();
            appendText(ss.str());
            read_offset = size;
        }
    }

    poll_ticks++;

    // Detect a missing/failed binary: either the log mentions it, or the
    // process died within the first couple of seconds without any output.
    std::ifstream check(log_file);
    std::string all((std::istreambuf_iterator<char>(check)), std::istreambuf_iterator<char>());
    bool looks_missing =
        all.find("No such file or directory") != std::string::npos ||
        all.find("command not found")         != std::string::npos ||
        all.find(": not found")               != std::string::npos;

    if (looks_missing) {
        showMissingBinaryBanner();
    } else if (poll_ticks >= 5 && all.empty() && !processStillAlive()) {
        // Process exited almost immediately with zero output — very likely
        // the binary itself is missing (shell reports 127 with no stderr
        // captured in some environments).
        showMissingBinaryBanner();
    }
}

gboolean LogMonitor::onPollTimeout(gpointer user_data) {
    LogMonitor* self = (LogMonitor*)user_data;
    self->pollOnce();
    return G_SOURCE_CONTINUE;
}

void LogMonitor::onWindowDestroy(GtkWidget*, gpointer user_data) {
    LogMonitor* self = (LogMonitor*)user_data;
    if (self->poll_source) { g_source_remove(self->poll_source); self->poll_source = 0; }
    delete self;
}

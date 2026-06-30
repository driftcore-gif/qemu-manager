#pragma once
#include <gtk/gtk.h>
#include <string>
#include <thread>
#include <atomic>

class QemuMonitor {
public:
    QemuMonitor(GtkWindow* parent, const std::string& vm_name,
                const std::string& monitor_sock_path);
    ~QemuMonitor();
    void show();

private:
    GtkWidget*  window      = nullptr;
    GtkWidget*  output_view = nullptr;
    GtkWidget*  input_entry = nullptr;
    GtkWidget*  status_lbl  = nullptr;

    std::string vm_name, sock_path;
    int         sock_fd  = -1;
    bool        connected = false;

    std::thread      reader_thread;
    std::atomic<bool> running{false};

    struct AppendData { QemuMonitor* self; std::string text; bool is_cmd; };
    struct QuickCmd   { const char* label; const char* cmd; };
    static const QuickCmd QUICK_CMDS[];

    void buildUI();
    bool connectSocket();
    void disconnectSocket();
    void sendCommand(const std::string& cmd);
    void appendOutput(const std::string& text, bool is_cmd = false);
    void readerLoop();
    void updateStatus(bool conn);

    static void     onSendClicked(GtkButton*, gpointer);
    static void     onInputActivate(GtkEntry*, gpointer);
    static void     onQuickCmd(GtkButton*, gpointer);
    static void     onDisconnect(GtkButton*, gpointer);
    static void     onReconnect(GtkButton*, gpointer);
    static void     onWindowDestroy(GtkWidget*, gpointer);
    static gboolean appendIdle(gpointer);
};

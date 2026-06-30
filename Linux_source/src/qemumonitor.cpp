#include "qemumonitor.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

const QemuMonitor::QuickCmd QemuMonitor::QUICK_CMDS[] = {
    {"Status",       "info status"},
    {"CPU Info",     "info cpus"},
    {"Mem",          "info balloon"},
    {"Block",        "info block"},
    {"Net",          "info network"},
    {"PCI",          "info pci"},
    {"Snapshots",    "info snapshots"},
    {"Registers",    "info registers"},
    {"Pause",        "stop"},
    {"Resume",       "cont"},
    {"Reset",        "system_reset"},
    {"Powerdown",    "system_powerdown"},
    {"Save State",   "savevm quicksave"},
    {"Load State",   "loadvm quicksave"},
    {"Screenshot",   "screendump /tmp/vm-screenshot.ppm"},
    {nullptr, nullptr}
};

QemuMonitor::QemuMonitor(GtkWindow* parent, const std::string& vm_name_,
                         const std::string& sock_path_)
    : vm_name(vm_name_), sock_path(sock_path_) {
    window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), ("QEMU Monitor — " + vm_name).c_str());
    gtk_window_set_transient_for(GTK_WINDOW(window), parent);
    gtk_window_set_default_size(GTK_WINDOW(window), 860, 580);
    g_signal_connect(window, "destroy", G_CALLBACK(onWindowDestroy), this);
    buildUI();
}

QemuMonitor::~QemuMonitor() {
    running = false;
    disconnectSocket();
    if (reader_thread.joinable()) reader_thread.join();
}

void QemuMonitor::show() {
    gtk_window_present(GTK_WINDOW(window));
    if (connectSocket()) {
        appendOutput("[Connected to " + sock_path + "]\n");
        updateStatus(true);
        running = true;
        reader_thread = std::thread([this]{ readerLoop(); });
    } else {
        appendOutput("[Could not connect to " + sock_path + "]\n"
                     "[Make sure the VM is running first]\n");
        updateStatus(false);
    }
}

void QemuMonitor::buildUI() {
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    // Top bar
    GtkWidget* top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(top, 10); gtk_widget_set_margin_end(top, 10);
    gtk_widget_set_margin_top(top, 8);   gtk_widget_set_margin_bottom(top, 4);

    GtkWidget* title = gtk_label_new(("Monitor: " + vm_name).c_str());
    PangoAttrList* al = pango_attr_list_new();
    pango_attr_list_insert(al, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(title), al);
    pango_attr_list_unref(al);
    gtk_widget_set_hexpand(title, TRUE);
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);

    status_lbl = gtk_label_new("● Disconnected");
    GtkWidget* rbtn = gtk_button_new_with_label("Reconnect");
    GtkWidget* dbtn = gtk_button_new_with_label("Disconnect");
    gtk_widget_add_css_class(dbtn, "destructive-action");
    g_signal_connect(rbtn, "clicked", G_CALLBACK(onReconnect),  this);
    g_signal_connect(dbtn, "clicked", G_CALLBACK(onDisconnect), this);

    gtk_box_append(GTK_BOX(top), title);
    gtk_box_append(GTK_BOX(top), status_lbl);
    gtk_box_append(GTK_BOX(top), rbtn);
    gtk_box_append(GTK_BOX(top), dbtn);
    gtk_box_append(GTK_BOX(vbox), top);

    // Quick buttons
    GtkWidget* qscroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(qscroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_widget_set_margin_start(qscroll, 6); gtk_widget_set_margin_end(qscroll, 6);
    GtkWidget* qbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_top(qbar, 4); gtk_widget_set_margin_bottom(qbar, 4);
    gtk_widget_set_margin_start(qbar, 4);
    for (int i = 0; QUICK_CMDS[i].label; i++) {
        GtkWidget* b = gtk_button_new_with_label(QUICK_CMDS[i].label);
        gtk_widget_add_css_class(b, "flat");
        g_object_set_data_full(G_OBJECT(b), "qcmd",
            g_strdup(QUICK_CMDS[i].cmd), g_free);
        g_signal_connect(b, "clicked", G_CALLBACK(onQuickCmd), this);
        gtk_box_append(GTK_BOX(qbar), b);
    }
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(qscroll), qbar);
    gtk_box_append(GTK_BOX(vbox), qscroll);
    gtk_box_append(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // Output area (dark terminal)
    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    output_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(output_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(output_view), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(output_view), GTK_WRAP_WORD_CHAR);
    gtk_widget_set_margin_start(output_view, 8);
    gtk_widget_set_margin_top(output_view, 6);

    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "textview,textview text{background:#1e1e1e;color:#d4d4d4;}", -1);
    gtk_style_context_add_provider(gtk_widget_get_style_context(output_view),
        GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_view));
    gtk_text_buffer_create_tag(buf, "cmd",   "foreground","#9cdcfe","weight",PANGO_WEIGHT_BOLD,nullptr);
    gtk_text_buffer_create_tag(buf, "out",   "foreground","#d4d4d4",nullptr);
    gtk_text_buffer_create_tag(buf, "err",   "foreground","#f44747",nullptr);
    gtk_text_buffer_create_tag(buf, "sys",   "foreground","#608b4e",nullptr);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), output_view);
    gtk_box_append(GTK_BOX(vbox), scroll);
    gtk_box_append(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // Input bar
    GtkWidget* ibar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(ibar,8); gtk_widget_set_margin_end(ibar,8);
    gtk_widget_set_margin_top(ibar,6);  gtk_widget_set_margin_bottom(ibar,8);

    GtkWidget* prompt = gtk_label_new("(qemu)");
    gtk_widget_add_css_class(prompt, "monospace");

    input_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(input_entry),
        "QEMU monitor command… (Enter to send)");
    gtk_widget_set_hexpand(input_entry, TRUE);

    GtkWidget* send = gtk_button_new_with_label("Send");
    gtk_widget_add_css_class(send, "suggested-action");
    g_signal_connect(send,        "clicked",  G_CALLBACK(onSendClicked),   this);
    g_signal_connect(input_entry, "activate", G_CALLBACK(onInputActivate), this);

    gtk_box_append(GTK_BOX(ibar), prompt);
    gtk_box_append(GTK_BOX(ibar), input_entry);
    gtk_box_append(GTK_BOX(ibar), send);
    gtk_box_append(GTK_BOX(vbox), ibar);
}

bool QemuMonitor::connectSocket() {
    sock_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) return false;
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path)-1);
    if (::connect(sock_fd,(struct sockaddr*)&addr,sizeof(addr)) < 0) {
        ::close(sock_fd); sock_fd = -1; return false;
    }
    connected = true; return true;
}

void QemuMonitor::disconnectSocket() {
    connected = false;
    if (sock_fd >= 0) { ::close(sock_fd); sock_fd = -1; }
}

void QemuMonitor::sendCommand(const std::string& cmd) {
    if (!connected || sock_fd < 0) { appendOutput("[Not connected]\n"); return; }
    appendOutput("(qemu) " + cmd + "\n", true);
    std::string l = cmd + "\n";
    ::send(sock_fd, l.c_str(), l.size(), 0);
}

void QemuMonitor::readerLoop() {
    char buf[4096];
    while (running && sock_fd >= 0) {
        ssize_t n = ::recv(sock_fd, buf, sizeof(buf)-1, 0);
        if (n <= 0) {
            if (running) appendOutput("\n[Disconnected by QEMU]\n");
            g_idle_add(+[](gpointer d)->gboolean{
                ((QemuMonitor*)d)->updateStatus(false); return G_SOURCE_REMOVE;
            }, this);
            break;
        }
        buf[n] = '\0';
        appendOutput(std::string(buf,n));
    }
}

gboolean QemuMonitor::appendIdle(gpointer data) {
    AppendData* ad = (AppendData*)data;
    GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ad->self->output_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);
    const char* tag = ad->is_cmd ? "cmd" : "out";
    gtk_text_buffer_insert_with_tags_by_name(buf, &end,
        ad->text.c_str(), -1, tag, nullptr);
    GtkTextMark* m = gtk_text_buffer_get_mark(buf,"insert");
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(ad->self->output_view),m,0,FALSE,0,0);
    delete ad; return G_SOURCE_REMOVE;
}

void QemuMonitor::appendOutput(const std::string& text, bool is_cmd) {
    g_idle_add(appendIdle, new AppendData{this, text, is_cmd});
}

void QemuMonitor::updateStatus(bool conn) {
    connected = conn;
    gtk_label_set_text(GTK_LABEL(status_lbl),
        conn ? "● Connected" : "● Disconnected");
}

void QemuMonitor::onSendClicked(GtkButton*, gpointer d) {
    auto* s = (QemuMonitor*)d;
    const char* t = gtk_editable_get_text(GTK_EDITABLE(s->input_entry));
    if (t && *t) { s->sendCommand(t); gtk_editable_set_text(GTK_EDITABLE(s->input_entry),""); }
}
void QemuMonitor::onInputActivate(GtkEntry* e, gpointer d) {
    auto* s = (QemuMonitor*)d;
    const char* t = gtk_editable_get_text(GTK_EDITABLE(e));
    if (t && *t) { s->sendCommand(t); gtk_editable_set_text(GTK_EDITABLE(e),""); }
}
void QemuMonitor::onQuickCmd(GtkButton* b, gpointer d) {
    auto* s = (QemuMonitor*)d;
    const char* cmd = (const char*)g_object_get_data(G_OBJECT(b),"qcmd");
    if (cmd) s->sendCommand(cmd);
}
void QemuMonitor::onDisconnect(GtkButton*, gpointer d) {
    auto* s = (QemuMonitor*)d;
    s->running = false; s->disconnectSocket();
    if (s->reader_thread.joinable()) s->reader_thread.join();
    s->appendOutput("[Disconnected]\n"); s->updateStatus(false);
}
void QemuMonitor::onReconnect(GtkButton*, gpointer d) {
    auto* s = (QemuMonitor*)d;
    s->running = false; s->disconnectSocket();
    if (s->reader_thread.joinable()) s->reader_thread.join();
    if (s->connectSocket()) {
        s->appendOutput("[Reconnected]\n"); s->updateStatus(true);
        s->running = true;
        s->reader_thread = std::thread([s]{ s->readerLoop(); });
    } else {
        s->appendOutput("[Reconnect failed — is the VM running?]\n");
    }
}
void QemuMonitor::onWindowDestroy(GtkWidget*, gpointer d) {
    auto* s = (QemuMonitor*)d;
    s->running = false; s->disconnectSocket();
    if (s->reader_thread.joinable()) s->reader_thread.detach();
    delete s;
}

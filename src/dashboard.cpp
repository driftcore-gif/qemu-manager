#include <gtk/gtk.h>
#include <string>
#include <fstream>
#include <sstream>
#include <array>
#include <memory>

static std::string exec_cmd(const std::string& cmd) {
    std::array<char,256> buf;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(),"r"), pclose);
    if (!pipe) return "N/A";
    while (fgets(buf.data(), buf.size(), pipe.get()) != nullptr)
        result += buf.data();
    if (!result.empty() && result.back()=='\n') result.pop_back();
    return result;
}

static std::string getCpuUsage() {
    // Read /proc/stat twice
    return exec_cmd("grep 'cpu ' /proc/stat | awk '{usage=($2+$4)*100/($2+$3+$4+$5)} END {printf \"%.1f%%\", usage}'");
}

static std::string getRamInfo() {
    return exec_cmd("free -m | awk '/Mem:/{printf \"%dMB / %dMB\", $3, $2}'");
}

static std::string getQemuVersion() {
    return exec_cmd("qemu-system-x86_64 --version 2>/dev/null | head -1");
}

static std::string getArchSupport() {
    return exec_cmd("ls /usr/bin/qemu-system-* 2>/dev/null | sed 's|/usr/bin/qemu-system-||' | tr '\\n' ' ' | cut -c1-60");
}

GtkWidget* buildDashboardWidget(int running, int stopped) {
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_start(grid, 16);
    gtk_widget_set_margin_end(grid, 16);
    gtk_widget_set_margin_top(grid, 16);
    gtk_widget_set_margin_bottom(grid, 16);

    auto make_card = [](const std::string& title, const std::string& value, const std::string& sub) -> GtkWidget* {
        GtkWidget* frame = gtk_frame_new(nullptr);
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_widget_set_margin_start(box, 16);
        gtk_widget_set_margin_end(box, 16);
        gtk_widget_set_margin_top(box, 12);
        gtk_widget_set_margin_bottom(box, 12);

        GtkWidget* lbl_title = gtk_label_new(title.c_str());
        gtk_widget_add_css_class(lbl_title, "dim-label");
        gtk_label_set_xalign(GTK_LABEL(lbl_title), 0.0f);

        GtkWidget* lbl_val = gtk_label_new(value.c_str());
        PangoAttrList* attrs = pango_attr_list_new();
        pango_attr_list_insert(attrs, pango_attr_scale_new(1.6));
        pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
        gtk_label_set_attributes(GTK_LABEL(lbl_val), attrs);
        pango_attr_list_unref(attrs);
        gtk_label_set_xalign(GTK_LABEL(lbl_val), 0.0f);

        GtkWidget* lbl_sub = gtk_label_new(sub.c_str());
        gtk_widget_add_css_class(lbl_sub, "dim-label");
        gtk_label_set_xalign(GTK_LABEL(lbl_sub), 0.0f);

        gtk_box_append(GTK_BOX(box), lbl_title);
        gtk_box_append(GTK_BOX(box), lbl_val);
        gtk_box_append(GTK_BOX(box), lbl_sub);
        gtk_frame_set_child(GTK_FRAME(frame), box);
        gtk_widget_set_hexpand(frame, TRUE);
        return frame;
    };

    std::string cpu = getCpuUsage();
    std::string ram = getRamInfo();
    std::string qver = getQemuVersion();
    std::string arch = getArchSupport();

    gtk_grid_attach(GTK_GRID(grid), make_card("CPU Usage", cpu, "Current system load"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_card("RAM Usage", ram, "Used / Total"), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_card("Running VMs", std::to_string(running), "Active virtual machines"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_card("Stopped VMs", std::to_string(stopped), "Inactive virtual machines"), 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_card("QEMU Version", qver.empty() ? "Not found" : qver, "Installed QEMU"), 0, 2, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), make_card("Arch Support", arch.empty() ? "None detected" : arch, "Available QEMU system targets"), 0, 3, 2, 1);

    return grid;
}

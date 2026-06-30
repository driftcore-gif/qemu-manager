#include "vmwizard.h"
#include "commandbuilder.h"
#include "vmconfig_io.h"
#include "storagemanager.h"
#include "settings.h"
#include "gtk_compat.h"
#include <string>
#include <glib/gstdio.h>

static const char* ARCH_NAMES[] = {
    "x86_64","i386",
    "aarch64","arm","armeb",
    "riscv64","riscv32",
    "mips","mipsel","mips64","mips64el","mipsn32","mipsn32el",
    "ppc","ppc64","ppc64le",
    "sparc","sparc64","sparc32plus",
    "alpha","hppa","m68k",
    "microblaze","microblazeel",
    "or1k","loongarch64","s390x",
    "sh4","sh4eb","xtensa","xtensaeb",
    "tricore","rx","avr","hexagon",
    nullptr
};

// ---- async ISO picker (GTK 4.10+) ----
#if GTK_CHECK_VERSION(4, 10, 0)
static void iso_open_cb(GObject* src, GAsyncResult* res, gpointer ud) {
    GtkEntry* entry = (GtkEntry*)ud;
    GFile* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(src), res, nullptr);
    if (file) {
        char* p = g_file_get_path(file);
        if (p) { gtk_editable_set_text(GTK_EDITABLE(entry), p); g_free(p); }
        g_object_unref(file);
    }
}
#endif

static GtkWidget* make_label(const char* t) {
    GtkWidget* l = gtk_label_new(t);
    gtk_label_set_xalign(GTK_LABEL(l), 1.0f);
    return l;
}

// ============================================================
VMWizard::VMWizard(GtkWindow* parent, std::function<void(VMConfig)> cb)
    : on_create_cb(cb) {
    dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "New Virtual Machine");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 760, 620);
    buildPages();
}

void VMWizard::show() { gtk_window_present(GTK_WINDOW(dialog)); }

// ============================================================
// General page
// ============================================================
GtkWidget* VMWizard::buildGeneralPage() {
    GtkWidget* g = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(g),10);
    gtk_grid_set_column_spacing(GTK_GRID(g),12);
    gtk_widget_set_margin_start(g,20); gtk_widget_set_margin_end(g,20);
    gtk_widget_set_margin_top(g,16);
    int r=0;
    auto row=[&](const char* lbl, GtkWidget* w){
        gtk_grid_attach(GTK_GRID(g),make_label(lbl),0,r,1,1);
        gtk_widget_set_hexpand(w,TRUE);
        gtk_grid_attach(GTK_GRID(g),w,1,r,1,1);
        const_cast<int&>(r)++;
    };
    name_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(name_entry),"e.g. Ubuntu-ARM");
    row("VM Name:", name_entry);
    desc_entry = gtk_entry_new();
    row("Description:", desc_entry);
    arch_combo = gtk_drop_down_new_from_strings(ARCH_NAMES);
    row("Architecture:", arch_combo);
    static const char* mode_names[] = {"System (softmmu)","User mode (linux-user)",nullptr};
    mode_combo = gtk_drop_down_new_from_strings(mode_names);
    row("Mode:", mode_combo);
    static const char* mach_names[] = {"q35","pc","virt","microvm","custom",nullptr};
    mach_combo = gtk_drop_down_new_from_strings(mach_names);
    row("Machine:", mach_combo);
    mach_custom = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(mach_custom),"e.g. raspi3b");
    gtk_widget_set_sensitive(mach_custom,FALSE);
    row("Custom Machine:", mach_custom);
    g_signal_connect(mach_combo,"notify::selected",
        G_CALLBACK(+[](GtkDropDown* dd,GParamSpec*,gpointer ud){
            gtk_widget_set_sensitive((GtkWidget*)ud,gtk_drop_down_get_selected(dd)==4);
        }),mach_custom);
    return g;
}

// ============================================================
// CPU page
// ============================================================
GtkWidget* VMWizard::buildCPUPage() {
    GtkWidget* g = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(g),10);
    gtk_grid_set_column_spacing(GTK_GRID(g),12);
    gtk_widget_set_margin_start(g,20); gtk_widget_set_margin_end(g,20);
    gtk_widget_set_margin_top(g,16);
    int r=0;
    auto row=[&](const char* lbl, GtkWidget* w){
        gtk_grid_attach(GTK_GRID(g),make_label(lbl),0,r,1,1);
        gtk_widget_set_hexpand(w,TRUE);
        gtk_grid_attach(GTK_GRID(g),w,1,r,1,1);
        const_cast<int&>(r)++;
    };
    cpu_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(cpu_entry),"max");
    gtk_entry_set_placeholder_text(GTK_ENTRY(cpu_entry),"max, host, qemu64, cortex-a53…");
    row("CPU Model:", cpu_entry);
    sockets_spin = gtk_spin_button_new_with_range(1,8,1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sockets_spin),1);
    row("Sockets:", sockets_spin);
    cores_spin = gtk_spin_button_new_with_range(1,64,1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(cores_spin),2);
    row("Cores:", cores_spin);
    threads_spin = gtk_spin_button_new_with_range(1,8,1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(threads_spin),2);
    row("Threads:", threads_spin);
    return g;
}

// ============================================================
// Memory page
// ============================================================
GtkWidget* VMWizard::buildMemoryPage() {
    GtkWidget* g = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(g),10);
    gtk_grid_set_column_spacing(GTK_GRID(g),12);
    gtk_widget_set_margin_start(g,20); gtk_widget_set_margin_end(g,20);
    gtk_widget_set_margin_top(g,16);
    ram_spin = gtk_spin_button_new_with_range(64,131072,256);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ram_spin),2048);
    gtk_widget_set_hexpand(ram_spin,TRUE);
    gtk_grid_attach(GTK_GRID(g),make_label("RAM (MB):"),0,0,1,1);
    gtk_grid_attach(GTK_GRID(g),ram_spin,1,0,1,1);
    balloon_check = gtk_check_button_new_with_label("Enable VirtIO Balloon");
    gtk_grid_attach(GTK_GRID(g),balloon_check,0,1,2,1);
    return g;
}

// ============================================================
// Storage page
// ============================================================
GtkWidget* VMWizard::buildStoragePage() {
    GtkWidget* g = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(g),10);
    gtk_grid_set_column_spacing(GTK_GRID(g),12);
    gtk_widget_set_margin_start(g,20); gtk_widget_set_margin_end(g,20);
    gtk_widget_set_margin_top(g,16);
    int r=0;
    auto row=[&](const char* lbl, GtkWidget* w){
        gtk_grid_attach(GTK_GRID(g),make_label(lbl),0,r,1,1);
        gtk_widget_set_hexpand(w,TRUE);
        gtk_grid_attach(GTK_GRID(g),w,1,r,1,1);
        const_cast<int&>(r)++;
    };
    disk_size_spin = gtk_spin_button_new_with_range(1,4000,1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(disk_size_spin),20);
    row("Disk Size (GB):", disk_size_spin);
    static const char* diskfmt_names[] = {"qcow2","raw","vmdk","vdi",nullptr};
    disk_fmt_combo = gtk_drop_down_new_from_strings(diskfmt_names);
    row("Disk Format:", disk_fmt_combo);
    GtkWidget* iso_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,4);
    iso_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(iso_entry),"/path/to/install.iso");
    gtk_widget_set_hexpand(iso_entry,TRUE);
    GtkWidget* browse = gtk_button_new_with_label("Browse…");
    g_signal_connect(browse,"clicked",G_CALLBACK(onBrowseISO),this);
    gtk_box_append(GTK_BOX(iso_box),iso_entry);
    gtk_box_append(GTK_BOX(iso_box),browse);
    gtk_widget_set_hexpand(iso_box,TRUE);
    gtk_grid_attach(GTK_GRID(g),make_label("ISO Path:"),0,r,1,1);
    gtk_grid_attach(GTK_GRID(g),iso_box,1,r,1,1);
    return g;
}

// ============================================================
// Boot page
// ============================================================
GtkWidget* VMWizard::buildBootPage() {
    GtkWidget* g = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(g),10);
    gtk_grid_set_column_spacing(GTK_GRID(g),12);
    gtk_widget_set_margin_start(g,20); gtk_widget_set_margin_end(g,20);
    gtk_widget_set_margin_top(g,16);
    uefi_check = gtk_check_button_new_with_label("Enable UEFI (requires OVMF)");
    gtk_grid_attach(GTK_GRID(g),uefi_check,0,0,2,1);
    boot_menu_check = gtk_check_button_new_with_label("Show boot menu on startup");
    gtk_grid_attach(GTK_GRID(g),boot_menu_check,0,1,2,1);
    gtk_grid_attach(GTK_GRID(g),make_label("Boot Order:"),0,2,1,1);
    static const char* boot_names[] = {"cd (CD first)","dc (Disk first)","c (Disk only)","d (CD only)",nullptr};
    boot_combo = gtk_drop_down_new_from_strings(boot_names);
    gtk_widget_set_hexpand(boot_combo,TRUE);
    gtk_grid_attach(GTK_GRID(g),boot_combo,1,2,1,1);
    return g;
}

// ============================================================
// Display page
// ============================================================
GtkWidget* VMWizard::buildDisplayPage() {
    GtkWidget* g = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(g),10);
    gtk_grid_set_column_spacing(GTK_GRID(g),12);
    gtk_widget_set_margin_start(g,20); gtk_widget_set_margin_end(g,20);
    gtk_widget_set_margin_top(g,16);
    int r=0;
    auto row=[&](const char* lbl, GtkWidget* w){
        gtk_grid_attach(GTK_GRID(g),make_label(lbl),0,r,1,1);
        gtk_widget_set_hexpand(w,TRUE);
        gtk_grid_attach(GTK_GRID(g),w,1,r,1,1);
        const_cast<int&>(r)++;
    };
    static const char* disp_names[] = {"SDL (recommended)","GTK","SPICE","VNC","EGL","Headless",nullptr};
    display_combo = gtk_drop_down_new_from_strings(disp_names);
    row("Display:", display_combo);
    static const char* gpu_names[] = {"VGA","VirtIO GPU","QXL","Cirrus","VMware SVGA","None",nullptr};
    gpu_combo = gtk_drop_down_new_from_strings(gpu_names);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(gpu_combo),1);
    row("GPU:", gpu_combo);
    static const char* audio_names[] = {"Intel HDA","AC97","SB16","None",nullptr};
    audio_combo = gtk_drop_down_new_from_strings(audio_names);
    row("Audio:", audio_combo);
    return g;
}

// ============================================================
// Network page
// ============================================================
GtkWidget* VMWizard::buildNetworkPage() {
    GtkWidget* g = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(g),10);
    gtk_grid_set_column_spacing(GTK_GRID(g),12);
    gtk_widget_set_margin_start(g,20); gtk_widget_set_margin_end(g,20);
    gtk_widget_set_margin_top(g,16);
    gtk_grid_attach(GTK_GRID(g),make_label("Network Mode:"),0,0,1,1);
    static const char* net_names[] = {"User (NAT — no root)","TAP","Bridge","Socket",nullptr};
    net_combo = gtk_drop_down_new_from_strings(net_names);
    gtk_widget_set_hexpand(net_combo,TRUE);
    gtk_grid_attach(GTK_GRID(g),net_combo,1,0,1,1);
    GtkWidget* note = gtk_label_new("User mode works without root on PRoot/Termux.");
    gtk_label_set_wrap(GTK_LABEL(note),TRUE);
    gtk_widget_add_css_class(note,"dim-label");
    gtk_label_set_xalign(GTK_LABEL(note),0);
    gtk_grid_attach(GTK_GRID(g),note,0,1,2,1);
    return g;
}

// ============================================================
// Hardware page  (binary + accelerator + extra args)
// ============================================================
GtkWidget* VMWizard::buildHardwarePage() {
    GtkWidget* g = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(g),12);
    gtk_grid_set_column_spacing(GTK_GRID(g),12);
    gtk_widget_set_margin_start(g,20); gtk_widget_set_margin_end(g,20);
    gtk_widget_set_margin_top(g,16);
    int r=0;

    // ---- QEMU Binary ----
    GtkWidget* bin_sect = gtk_label_new("QEMU Binary");
    PangoAttrList* al = pango_attr_list_new();
    pango_attr_list_insert(al, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(bin_sect), al);
    pango_attr_list_unref(al);
    gtk_label_set_xalign(GTK_LABEL(bin_sect),0);
    gtk_grid_attach(GTK_GRID(g),bin_sect,0,r,2,1); r++;

    // Scan /usr/bin for installed qemu-system-* binaries
    auto bins = CommandBuilder::listInstalledBinaries(VMMode::System);
    // Build string list for dropdown: detected binaries + "Custom…"
    std::vector<const char*> bin_list;
    std::vector<std::string> bin_strings = bins;
    bin_strings.push_back("Custom…");
    for (const auto& b : bin_strings) bin_list.push_back(b.c_str());
    bin_list.push_back(nullptr);

    bin_combo = gtk_drop_down_new_from_strings(bin_list.data());
    gtk_widget_set_hexpand(bin_combo,TRUE);
    gtk_grid_attach(GTK_GRID(g),make_label("Binary:"),0,r,1,1);
    gtk_grid_attach(GTK_GRID(g),bin_combo,1,r,1,1); r++;

    // Custom binary text box — shown only when "Custom…" is selected
    custom_bin_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(custom_bin_entry),
        "e.g.  qemu-system-mycpu   (name only, resolved in /usr/bin/)");
    gtk_widget_set_sensitive(custom_bin_entry, FALSE);
    gtk_widget_set_hexpand(custom_bin_entry, TRUE);
    gtk_grid_attach(GTK_GRID(g),make_label("Custom Name:"),0,r,1,1);
    gtk_grid_attach(GTK_GRID(g),custom_bin_entry,1,r,1,1); r++;

    // Validation hint label
    bin_hint_lbl = gtk_label_new("");
    gtk_widget_add_css_class(bin_hint_lbl,"dim-label");
    gtk_label_set_xalign(GTK_LABEL(bin_hint_lbl),0);
    gtk_grid_attach(GTK_GRID(g),bin_hint_lbl,0,r,2,1); r++;

    // Toggle custom entry visibility
    // Store bins count so we know which index is "Custom…"
    g_object_set_data(G_OBJECT(bin_combo), "custom_idx",
        GINT_TO_POINTER((int)bins.size()));
    g_object_set_data(G_OBJECT(bin_combo), "entry", custom_bin_entry);
    g_object_set_data(G_OBJECT(bin_combo), "hint",  bin_hint_lbl);
    g_signal_connect(bin_combo,"notify::selected",
        G_CALLBACK(+[](GtkDropDown* dd, GParamSpec*, gpointer){
            int custom_idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dd),"custom_idx"));
            GtkWidget* ent = (GtkWidget*)g_object_get_data(G_OBJECT(dd),"entry");
            bool is_custom = (int)gtk_drop_down_get_selected(dd) == custom_idx;
            gtk_widget_set_sensitive(ent, is_custom);
            if (!is_custom) {
                GtkWidget* hint = (GtkWidget*)g_object_get_data(G_OBJECT(dd),"hint");
                gtk_label_set_text(GTK_LABEL(hint),"");
            }
        }), nullptr);

    // Validate on typing — only binary name allowed
    g_signal_connect(custom_bin_entry,"changed",
        G_CALLBACK(+[](GtkEditable* ed, gpointer ud){
            GtkLabel* hint = GTK_LABEL(ud);
            const char* txt = gtk_editable_get_text(ed);
            std::string err = CommandBuilder::validateCustomBinary(
                txt ? txt : "");
            // Don't check file existence while typing
            if (err == ("Binary '/usr/bin/" + std::string(txt?txt:"") + "' not found."))
                err = "";
            // Block spaces immediately
            std::string s = txt ? txt : "";
            if (s.find(' ')!=std::string::npos || s.find('/')!=std::string::npos) {
                gtk_label_set_text(hint,"⚠ Binary name only — no spaces or paths.");
                gtk_widget_add_css_class(GTK_WIDGET(ed),"error");
            } else {
                gtk_label_set_text(hint, err.empty() ? "" : ("⚠ " + err).c_str());
                gtk_widget_remove_css_class(GTK_WIDGET(ed),"error");
            }
        }), bin_hint_lbl);

    gtk_grid_attach(GTK_GRID(g),gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),0,r,2,1); r++;

    // ---- Accelerator ----
    GtkWidget* acc_sect = gtk_label_new("Accelerator");
    PangoAttrList* al2 = pango_attr_list_new();
    pango_attr_list_insert(al2, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(acc_sect), al2);
    pango_attr_list_unref(al2);
    gtk_label_set_xalign(GTK_LABEL(acc_sect),0);
    gtk_grid_attach(GTK_GRID(g),acc_sect,0,r,2,1); r++;

    static const char* accel_names[] = {
            "TCG  (software emulation — works everywhere)",
            "KVM  (hardware acceleration — requires /dev/kvm)",
            "KVM + LBT  (LoongArch silicon binary translation)",
            nullptr
        };
    accel_combo = gtk_drop_down_new_from_strings(accel_names);
    gtk_widget_set_hexpand(accel_combo,TRUE);
    gtk_grid_attach(GTK_GRID(g),make_label("Accelerator:"),0,r,1,1);
    gtk_grid_attach(GTK_GRID(g),accel_combo,1,r,1,1); r++;

    // LoongArch LBT note — shown when KVM+LBT selected
    lbt_note_lbl = gtk_label_new(
        "LoongArch LBT: silicon-level x86/MIPS/ARM translation.\n"
        "Requires LoongArch hardware with LBT extension + KVM enabled kernel.");
    gtk_label_set_wrap(GTK_LABEL(lbt_note_lbl),TRUE);
    gtk_widget_add_css_class(lbt_note_lbl,"dim-label");
    gtk_label_set_xalign(GTK_LABEL(lbt_note_lbl),0);
    gtk_widget_set_visible(lbt_note_lbl,FALSE);
    gtk_grid_attach(GTK_GRID(g),lbt_note_lbl,0,r,2,1); r++;

    g_signal_connect(accel_combo,"notify::selected",
        G_CALLBACK(+[](GtkDropDown* dd, GParamSpec*, gpointer ud){
            gtk_widget_set_visible((GtkWidget*)ud,
                gtk_drop_down_get_selected(dd)==2);
        }), lbt_note_lbl);

    gtk_grid_attach(GTK_GRID(g),gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),0,r,2,1); r++;

    // ---- Extra Arguments ----
    GtkWidget* ex_sect = gtk_label_new("Extra Arguments");
    PangoAttrList* al3 = pango_attr_list_new();
    pango_attr_list_insert(al3, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(ex_sect), al3);
    pango_attr_list_unref(al3);
    gtk_label_set_xalign(GTK_LABEL(ex_sect),0);
    gtk_grid_attach(GTK_GRID(g),ex_sect,0,r,2,1); r++;

    extra_args_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(extra_args_entry),
        "-device virtio-rng-pci   -no-reboot   -snapshot");
    gtk_widget_set_hexpand(extra_args_entry,TRUE);
    gtk_grid_attach(GTK_GRID(g),make_label("Append:"),0,r,1,1);
    gtk_grid_attach(GTK_GRID(g),extra_args_entry,1,r,1,1); r++;

    GtkWidget* ex_hint = gtk_label_new(
        "These flags are appended after all generated arguments.");
    gtk_label_set_xalign(GTK_LABEL(ex_hint),0);
    gtk_widget_add_css_class(ex_hint,"dim-label");
    gtk_grid_attach(GTK_GRID(g),ex_hint,0,r,2,1); r++;

    gtk_grid_attach(GTK_GRID(g),gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),0,r,2,1); r++;

    // ---- Save start.sh ----
    save_script_check = gtk_check_button_new_with_label(
        "Save start.sh to VM folder  (~/VMs/<name>/start.sh)");
    gtk_grid_attach(GTK_GRID(g),save_script_check,0,r,2,1); r++;

    GtkWidget* sh_hint = gtk_label_new(
        "Unchecked: QEMU launched directly in RAM — no file written.\n"
        "Checked: start.sh also written to VM folder for manual reuse.");
    gtk_label_set_wrap(GTK_LABEL(sh_hint),TRUE);
    gtk_widget_add_css_class(sh_hint,"dim-label");
    gtk_label_set_xalign(GTK_LABEL(sh_hint),0);
    gtk_grid_attach(GTK_GRID(g),sh_hint,0,r,2,1);

    return g;
}

// ============================================================
// Preview page
// ============================================================
GtkWidget* VMWizard::buildPreviewPage() {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL,8);
    gtk_widget_set_margin_start(box,16); gtk_widget_set_margin_end(box,16);
    gtk_widget_set_margin_top(box,12);
    GtkWidget* lbl = gtk_label_new("Generated QEMU Command:");
    gtk_label_set_xalign(GTK_LABEL(lbl),0);
    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll,TRUE);
    cmd_preview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(cmd_preview),FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(cmd_preview),TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(cmd_preview),GTK_WRAP_WORD_CHAR);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll),cmd_preview);
    gtk_box_append(GTK_BOX(box),lbl);
    gtk_box_append(GTK_BOX(box),scroll);
    return box;
}

void VMWizard::updatePreview() {
    VMConfig c = collectConfig();
    std::string cmd = CommandBuilder::formatCommand(c);
    GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(cmd_preview));
    gtk_text_buffer_set_text(buf,cmd.c_str(),-1);
}

// ============================================================
// Collect all widget values into VMConfig
// ============================================================
VMConfig VMWizard::collectConfig() {
    VMConfig c;
    c.name        = gtk_editable_get_text(GTK_EDITABLE(name_entry));
    c.description = gtk_editable_get_text(GTK_EDITABLE(desc_entry));
    c.arch        = (VMArch)gtk_drop_down_get_selected(GTK_DROP_DOWN(arch_combo));
    c.mode        = gtk_drop_down_get_selected(GTK_DROP_DOWN(mode_combo))==1
                    ? VMMode::UserMode : VMMode::System;
    int mi = gtk_drop_down_get_selected(GTK_DROP_DOWN(mach_combo));
    const MachineType mt[]={MachineType::q35,MachineType::pc,MachineType::virt,
                             MachineType::microvm,MachineType::custom};
    c.machine = mt[mi<5?mi:0];
    if (c.machine==MachineType::custom)
        c.machine_custom = gtk_editable_get_text(GTK_EDITABLE(mach_custom));
    c.cpu_model = gtk_editable_get_text(GTK_EDITABLE(cpu_entry));
    c.sockets   = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(sockets_spin));
    c.cores     = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(cores_spin));
    c.threads   = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(threads_spin));
    c.ram_mb    = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(ram_spin));
    c.ballooning = gtk_check_button_get_active(GTK_CHECK_BUTTON(balloon_check));
    c.disk_size_gb = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(disk_size_spin));
    c.disk_format  = (DiskFormat)gtk_drop_down_get_selected(GTK_DROP_DOWN(disk_fmt_combo));
    c.iso_path     = gtk_editable_get_text(GTK_EDITABLE(iso_entry));
    c.uefi         = gtk_check_button_get_active(GTK_CHECK_BUTTON(uefi_check));
    c.boot_menu    = gtk_check_button_get_active(GTK_CHECK_BUTTON(boot_menu_check));
    const char* bo[]={"cd","dc","c","d"};
    int boi = gtk_drop_down_get_selected(GTK_DROP_DOWN(boot_combo));
    c.boot_order = bo[boi<4?boi:0];

    // Display — SDL is index 0 in our display combo
    const DisplayType dt[]={DisplayType::SDL,DisplayType::GTK,DisplayType::SPICE,
                             DisplayType::VNC,DisplayType::EGL,DisplayType::Headless};
    c.display = dt[gtk_drop_down_get_selected(GTK_DROP_DOWN(display_combo))];
    c.gpu     = (GPUType)gtk_drop_down_get_selected(GTK_DROP_DOWN(gpu_combo));
    c.audio   = (AudioType)gtk_drop_down_get_selected(GTK_DROP_DOWN(audio_combo));
    c.net_mode = (NetworkMode)gtk_drop_down_get_selected(GTK_DROP_DOWN(net_combo));

    // Binary
    int custom_idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(bin_combo),"custom_idx"));
    int sel = (int)gtk_drop_down_get_selected(GTK_DROP_DOWN(bin_combo));
    if (sel == custom_idx) {
        c.binary_mode   = BinaryMode::Custom;
        c.custom_binary = gtk_editable_get_text(GTK_EDITABLE(custom_bin_entry));
    } else {
        c.binary_mode   = BinaryMode::Auto;
        c.custom_binary = "";
    }

    // Accelerator
    const Accelerator ac[]={Accelerator::TCG,Accelerator::KVM,Accelerator::KVM_LBT};
    int ai = gtk_drop_down_get_selected(GTK_DROP_DOWN(accel_combo));
    c.accel = ac[ai<3?ai:0];

    c.extra_args               = gtk_editable_get_text(GTK_EDITABLE(extra_args_entry));
    c.save_script_to_vm_folder = gtk_check_button_get_active(GTK_CHECK_BUTTON(save_script_check));

    const std::string& vf = Settings::get().vm_folder;
    c.vm_dir = vf + "/" + c.name;
    const char* exts[]={"qcow2","raw","vmdk","vdi"};
    int fi = (int)c.disk_format;
    c.disk_path = c.vm_dir+"/disk."+exts[fi<4?fi:0];
    return c;
}

// ============================================================
// Assemble notebook
// ============================================================
void VMWizard::buildPages() {
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_window_set_child(GTK_WINDOW(dialog),vbox);
    notebook = gtk_notebook_new();
    gtk_widget_set_vexpand(notebook,TRUE);
    auto add=[&](const char* lbl, GtkWidget* w){
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook),w,gtk_label_new(lbl));
    };
    add("General",  buildGeneralPage());
    add("CPU",      buildCPUPage());
    add("Memory",   buildMemoryPage());
    add("Storage",  buildStoragePage());
    add("Boot",     buildBootPage());
    add("Display",  buildDisplayPage());
    add("Network",  buildNetworkPage());
    add("Hardware", buildHardwarePage());   // binary + accel + extra args
    add("Preview",  buildPreviewPage());
    g_signal_connect(notebook,"switch-page",G_CALLBACK(onPageSwitch),this);
    gtk_box_append(GTK_BOX(vbox),notebook);
    GtkWidget* btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);
    gtk_widget_set_margin_start(btns,16); gtk_widget_set_margin_end(btns,16);
    gtk_widget_set_margin_top(btns,8);   gtk_widget_set_margin_bottom(btns,12);
    GtkWidget* sp = gtk_label_new(""); gtk_widget_set_hexpand(sp,TRUE);
    GtkWidget* cancel = gtk_button_new_with_label("Cancel");
    GtkWidget* create = gtk_button_new_with_label("Create VM");
    gtk_widget_add_css_class(create,"suggested-action");
    g_signal_connect(cancel,"clicked",G_CALLBACK(onCancelClicked),this);
    g_signal_connect(create,"clicked",G_CALLBACK(onCreateClicked),this);
    gtk_box_append(GTK_BOX(btns),sp);
    gtk_box_append(GTK_BOX(btns),cancel);
    gtk_box_append(GTK_BOX(btns),create);
    gtk_box_append(GTK_BOX(vbox),btns);
}

void VMWizard::onPageSwitch(GtkNotebook* nb,GtkWidget*,guint page,gpointer d) {
    VMWizard* s=(VMWizard*)d;
    if ((int)page==gtk_notebook_get_n_pages(nb)-1) s->updatePreview();
}

void VMWizard::onBrowseISO(GtkButton*,gpointer d) {
    VMWizard* s=(VMWizard*)d;
#if GTK_CHECK_VERSION(4, 10, 0)
    GtkFileDialog* dlg = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dlg,"Select ISO Image");
    GListStore* fs = g_list_store_new(GTK_TYPE_FILE_FILTER);
    GtkFileFilter* f = gtk_file_filter_new();
    gtk_file_filter_add_pattern(f,"*.iso");
    gtk_file_filter_set_name(f,"ISO Images");
    g_list_store_append(fs,f); g_object_unref(f);
    gtk_file_dialog_set_filters(dlg,G_LIST_MODEL(fs)); g_object_unref(fs);
    gtk_file_dialog_open(dlg,GTK_WINDOW(s->dialog),nullptr,iso_open_cb,s->iso_entry);
    g_object_unref(dlg);
#else
    compat_open_file_chooser(GTK_WINDOW(s->dialog), "Select ISO Image",
        GTK_ENTRY(s->iso_entry), "*.iso", "ISO Images");
#endif
}

void VMWizard::onCreateClicked(GtkButton*,gpointer d) {
    VMWizard* s=(VMWizard*)d;
    VMConfig c = s->collectConfig();
    if (c.name.empty()) {
#if GTK_CHECK_VERSION(4, 10, 0)
        GtkAlertDialog* ad = gtk_alert_dialog_new("Please enter a VM name.");
        gtk_alert_dialog_show(ad,GTK_WINDOW(s->dialog));
        g_object_unref(ad);
#else
        compat_show_alert(GTK_WINDOW(s->dialog), "Please enter a VM name.");
#endif
        return;
    }
    // Validate custom binary before creating
    if (c.binary_mode==BinaryMode::Custom) {
        std::string err = CommandBuilder::validateCustomBinary(c.custom_binary);
        if (!err.empty()) {
#if GTK_CHECK_VERSION(4, 10, 0)
            GtkAlertDialog* ad = gtk_alert_dialog_new(("Binary error: "+err).c_str());
            gtk_alert_dialog_show(ad,GTK_WINDOW(s->dialog));
            g_object_unref(ad);
#else
            compat_show_alert(GTK_WINDOW(s->dialog), ("Binary error: "+err).c_str());
#endif
            return;
        }
    }
    g_mkdir_with_parents(c.vm_dir.c_str(),0755);
    g_mkdir_with_parents((c.vm_dir+"/snapshots").c_str(),0755);
    g_mkdir_with_parents((c.vm_dir+"/logs").c_str(),0755);
    if (c.mode==VMMode::System)
        StorageManager::createDisk(c.disk_path,c.disk_format,c.disk_size_gb);
    VMConfigIO::save(c);
    if (c.save_script_to_vm_folder)
        CommandBuilder::writeStartScript(c);
    if (s->on_create_cb) s->on_create_cb(c);
    gtk_window_destroy(GTK_WINDOW(s->dialog));
    delete s;
}

void VMWizard::onCancelClicked(GtkButton*,gpointer d) {
    VMWizard* s=(VMWizard*)d;
    gtk_window_destroy(GTK_WINDOW(s->dialog));
    delete s;
}

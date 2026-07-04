#include "mainwindow.h"
#include "commandbuilder.h"
#include "vmconfig_io.h"
#include "settings.h"
#include "gtk_compat.h"
#include <string>
#include <glib/gstdio.h>
#include <fstream>

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

static GtkWidget* make_label(const char* t) {
    GtkWidget* l = gtk_label_new(t);
    gtk_label_set_xalign(GTK_LABEL(l), 1.0f);
    return l;
}

VMSettingsDialog::VMSettingsDialog(GtkWindow* parent, VMConfig& vm, std::function<void(VMConfig)> cb)
    : vm_ref(vm), on_save_cb(cb) {
    dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "VM Settings");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 760, 620);
    buildPages();
    populateFromVM();
}

void VMSettingsDialog::show() { gtk_window_present(GTK_WINDOW(dialog)); }

void VMSettingsDialog::populateFromVM() {
    gtk_editable_set_text(GTK_EDITABLE(name_entry), vm_ref.name.c_str());
    gtk_editable_set_text(GTK_EDITABLE(desc_entry), vm_ref.description.c_str());
    gtk_drop_down_set_selected(GTK_DROP_DOWN(arch_combo), (guint)vm_ref.arch);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(mode_combo), vm_ref.mode == VMMode::UserMode ? 1 : 0);
    
    const MachineType mt[] = {MachineType::q35,MachineType::pc,MachineType::virt,MachineType::microvm,MachineType::sbsa_ref,MachineType::virt_acpi,MachineType::x86_64_microvm,MachineType::nitro_enclave,MachineType::amd_versal2_virt,MachineType::custom};
    for (int i = 0; i < 5; i++) if (mt[i] == vm_ref.machine) { gtk_drop_down_set_selected(GTK_DROP_DOWN(mach_combo), i); break; }
    if (vm_ref.machine == MachineType::custom) {
        gtk_editable_set_text(GTK_EDITABLE(mach_custom), vm_ref.machine_custom.c_str());
        gtk_widget_set_sensitive(mach_custom, TRUE);
    }
    
    gtk_editable_set_text(GTK_EDITABLE(cpu_entry), vm_ref.cpu_model.c_str());
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sockets_spin), vm_ref.sockets);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(cores_spin), vm_ref.cores);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(threads_spin), vm_ref.threads);
    
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ram_spin), vm_ref.ram_mb);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(balloon_check), vm_ref.ballooning);
    
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(disk_size_spin), vm_ref.disk_size_gb);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(disk_fmt_combo), (guint)vm_ref.disk_format);
    gtk_editable_set_text(GTK_EDITABLE(iso_entry), vm_ref.iso_path.c_str());
    
    gtk_check_button_set_active(GTK_CHECK_BUTTON(uefi_check), (vm_ref.firmware==Firmware::OVMF||vm_ref.firmware==Firmware::OVMF_SecureBoot));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(boot_menu_check), vm_ref.boot_menu);
    const char* bo[] = {"cd","dc","c","d"};
    for (int i = 0; i < 4; i++) if (std::string(bo[i]) == vm_ref.boot_order) { gtk_drop_down_set_selected(GTK_DROP_DOWN(boot_combo), i); break; }
    
    const DisplayType dt[] = {DisplayType::SDL,DisplayType::GTK,DisplayType::SPICE,DisplayType::VNC,DisplayType::EGL,DisplayType::Headless};
    for (int i = 0; i < 6; i++) if (dt[i] == vm_ref.display) { gtk_drop_down_set_selected(GTK_DROP_DOWN(display_combo), i); break; }
    gtk_drop_down_set_selected(GTK_DROP_DOWN(gpu_combo), (guint)vm_ref.gpu);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(audio_combo), (guint)vm_ref.audio);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(net_combo), (guint)vm_ref.net_mode);
    
    gtk_drop_down_set_selected(GTK_DROP_DOWN(accel_combo), (guint)vm_ref.accel);
    if (vm_ref.accel == Accelerator::KVM_LBT) gtk_widget_set_visible(lbt_note_lbl, TRUE);
    
    gtk_editable_set_text(GTK_EDITABLE(extra_args_entry), vm_ref.extra_args.c_str());
    gtk_check_button_set_active(GTK_CHECK_BUTTON(save_script_check), vm_ref.save_script_to_vm_folder);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(kvm_cet_check), vm_ref.kvm_cet);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(conf_vm_combo), (guint)vm_ref.conf_vm);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(scsi_mq_check), vm_ref.scsi_multiqueue);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(riscv_iommu_check), vm_ref.riscv_iommu);
    // QEMU 10.2
    gtk_check_button_set_active(GTK_CHECK_BUTTON(io_uring_check), vm_ref.io_uring_loop);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(migration_combo), (guint)vm_ref.migration_mode);
    if(virtfs_path_entry && !vm_ref.virtfs_path.empty())
        gtk_editable_set_text(GTK_EDITABLE(virtfs_path_entry), vm_ref.virtfs_path.c_str());
    if(virtfs_tag_entry)
        gtk_editable_set_text(GTK_EDITABLE(virtfs_tag_entry), vm_ref.virtfs_mount_tag.c_str());
    
    if (vm_ref.binary_mode == BinaryMode::Custom) {
        gtk_editable_set_text(GTK_EDITABLE(custom_bin_entry), vm_ref.custom_binary.c_str());
    }
    
    updatePreview();
}

void VMSettingsDialog::buildPages() {
    // Similar to VMWizard but for editing
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(dialog), vbox);
    notebook = gtk_notebook_new();
    gtk_widget_set_vexpand(notebook, TRUE);
    
    auto add = [&](const char* lbl, GtkWidget* w) {
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, gtk_label_new(lbl));
    };
    
    // ---- General page ----
    {
        GtkWidget* g = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(g), 10);
        gtk_grid_set_column_spacing(GTK_GRID(g), 12);
        gtk_widget_set_margin_start(g, 20); gtk_widget_set_margin_end(g, 20);
        gtk_widget_set_margin_top(g, 16);
        int r = 0;
        auto row = [&](const char* lbl, GtkWidget* w) {
            gtk_grid_attach(GTK_GRID(g), make_label(lbl), 0, r, 1, 1);
            gtk_widget_set_hexpand(w, TRUE);
            gtk_grid_attach(GTK_GRID(g), w, 1, r, 1, 1);
            r++;
        };
        name_entry = gtk_entry_new();
        row("VM Name:", name_entry);
        desc_entry = gtk_entry_new();
        row("Description:", desc_entry);
        arch_combo = gtk_drop_down_new_from_strings(ARCH_NAMES);
        row("Architecture:", arch_combo);
        static const char* mode_names[] = {"System (softmmu)", "User mode (linux-user)", nullptr};
        mode_combo = gtk_drop_down_new_from_strings(mode_names);
        row("Mode:", mode_combo);
        static const char* mach_names[] = {"q35","pc","virt","microvm","sbsa-ref","virt+ACPI","x86-microvm","nitro-enclave","amd-versal2-virt","custom",nullptr};
        mach_combo = gtk_drop_down_new_from_strings(mach_names);
        row("Machine:", mach_combo);
        mach_custom = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(mach_custom), "e.g. raspi3b");
        gtk_widget_set_sensitive(mach_custom, FALSE);
        row("Custom Machine:", mach_custom);
        g_signal_connect(mach_combo, "notify::selected",
            G_CALLBACK(+[](GtkDropDown* dd, GParamSpec*, gpointer ud) {
                gtk_widget_set_sensitive((GtkWidget*)ud, gtk_drop_down_get_selected(dd) == 4);
            }), mach_custom);
        add("General", g);
    }
    
    // ---- CPU page ----
    {
        GtkWidget* g = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(g), 10);
        gtk_grid_set_column_spacing(GTK_GRID(g), 12);
        gtk_widget_set_margin_start(g, 20); gtk_widget_set_margin_end(g, 20);
        gtk_widget_set_margin_top(g, 16);
        int r = 0;
        auto row = [&](const char* lbl, GtkWidget* w) {
            gtk_grid_attach(GTK_GRID(g), make_label(lbl), 0, r, 1, 1);
            gtk_widget_set_hexpand(w, TRUE);
            gtk_grid_attach(GTK_GRID(g), w, 1, r, 1, 1);
            r++;
        };
        cpu_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(cpu_entry), "max, host, qemu64, cortex-a53...");
        row("CPU Model:", cpu_entry);
        sockets_spin = gtk_spin_button_new_with_range(1, 8, 1);
        row("Sockets:", sockets_spin);
        cores_spin = gtk_spin_button_new_with_range(1, 64, 1);
        row("Cores:", cores_spin);
        threads_spin = gtk_spin_button_new_with_range(1, 8, 1);
        row("Threads:", threads_spin);
        add("CPU", g);
    }
    
    // ---- Memory page ----
    {
        GtkWidget* g = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(g), 10);
        gtk_grid_set_column_spacing(GTK_GRID(g), 12);
        gtk_widget_set_margin_start(g, 20); gtk_widget_set_margin_end(g, 20);
        gtk_widget_set_margin_top(g, 16);
        ram_spin = gtk_spin_button_new_with_range(64, 131072, 256);
        gtk_widget_set_hexpand(ram_spin, TRUE);
        gtk_grid_attach(GTK_GRID(g), make_label("RAM (MB):"), 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(g), ram_spin, 1, 0, 1, 1);
        balloon_check = gtk_check_button_new_with_label("Enable VirtIO Balloon");
        gtk_grid_attach(GTK_GRID(g), balloon_check, 0, 1, 2, 1);
        add("Memory", g);
    }
    
    // ---- Storage page ----
    {
        GtkWidget* g = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(g), 10);
        gtk_grid_set_column_spacing(GTK_GRID(g), 12);
        gtk_widget_set_margin_start(g, 20); gtk_widget_set_margin_end(g, 20);
        gtk_widget_set_margin_top(g, 16);
        int r = 0;
        auto row = [&](const char* lbl, GtkWidget* w) {
            gtk_grid_attach(GTK_GRID(g), make_label(lbl), 0, r, 1, 1);
            gtk_widget_set_hexpand(w, TRUE);
            gtk_grid_attach(GTK_GRID(g), w, 1, r, 1, 1);
            r++;
        };
        disk_size_spin = gtk_spin_button_new_with_range(1, 4000, 1);
        row("Disk Size (GB):", disk_size_spin);
        static const char* diskfmt_names[] = {"qcow2","raw","vmdk","vdi",nullptr};
        disk_fmt_combo = gtk_drop_down_new_from_strings(diskfmt_names);
        row("Disk Format:", disk_fmt_combo);
        GtkWidget* iso_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        iso_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(iso_entry), "/path/to/install.iso");
        gtk_widget_set_hexpand(iso_entry, TRUE);
        GtkWidget* browse = gtk_button_new_with_label("Browse...");
        g_signal_connect(browse, "clicked", G_CALLBACK(onBrowseISO), this);
        gtk_box_append(GTK_BOX(iso_box), iso_entry);
        gtk_box_append(GTK_BOX(iso_box), browse);
        row("ISO Path:", iso_box);
        add("Storage", g);
    }
    
    // ---- Boot page ----
    {
        GtkWidget* g = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(g), 10);
        gtk_grid_set_column_spacing(GTK_GRID(g), 12);
        gtk_widget_set_margin_start(g, 20); gtk_widget_set_margin_end(g, 20);
        gtk_widget_set_margin_top(g, 16);
        uefi_check = gtk_check_button_new_with_label("Enable UEFI (requires OVMF)");
        gtk_grid_attach(GTK_GRID(g), uefi_check, 0, 0, 2, 1);
        boot_menu_check = gtk_check_button_new_with_label("Show boot menu on startup");
        gtk_grid_attach(GTK_GRID(g), boot_menu_check, 0, 1, 2, 1);
        gtk_grid_attach(GTK_GRID(g), make_label("Boot Order:"), 0, 2, 1, 1);
        static const char* boot_names[] = {"cd (CD first)","dc (Disk first)","c (Disk only)","d (CD only)",nullptr};
        boot_combo = gtk_drop_down_new_from_strings(boot_names);
        gtk_widget_set_hexpand(boot_combo, TRUE);
        gtk_grid_attach(GTK_GRID(g), boot_combo, 1, 2, 1, 1);
        add("Boot", g);
    }
    
    // ---- Display page ----
    {
        GtkWidget* g = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(g), 10);
        gtk_grid_set_column_spacing(GTK_GRID(g), 12);
        gtk_widget_set_margin_start(g, 20); gtk_widget_set_margin_end(g, 20);
        gtk_widget_set_margin_top(g, 16);
        int r = 0;
        auto row = [&](const char* lbl, GtkWidget* w) {
            gtk_grid_attach(GTK_GRID(g), make_label(lbl), 0, r, 1, 1);
            gtk_widget_set_hexpand(w, TRUE);
            gtk_grid_attach(GTK_GRID(g), w, 1, r, 1, 1);
            r++;
        };
        static const char* disp_names[] = {"SDL (recommended)","GTK","SPICE","VNC","EGL","Headless",nullptr};
        display_combo = gtk_drop_down_new_from_strings(disp_names);
        row("Display:", display_combo);
        static const char* gpu_names[] = {
            "VGA (standard)","VirtIO GPU (2D)","VirtIO GPU GL (VirGL/OpenGL)","VirtIO GPU Rutabaga (Android)","VirtIO GPU NativeCtx (QEMU 11)","QXL (SPICE)","Cirrus","VMware SVGA","ramfb","None",nullptr};
        gpu_combo = gtk_drop_down_new_from_strings(gpu_names);
        row("GPU:", gpu_combo);
        static const char* audio_names[] = {"Intel HDA","AC97","SB16","VirtIO Sound (QEMU 8+)","None",nullptr};
        audio_combo = gtk_drop_down_new_from_strings(audio_names);
        row("Audio:", audio_combo);
        add("Display", g);
    }
    
    // ---- Network page ----
    {
        GtkWidget* g = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(g), 10);
        gtk_grid_set_column_spacing(GTK_GRID(g), 12);
        gtk_widget_set_margin_start(g, 20); gtk_widget_set_margin_end(g, 20);
        gtk_widget_set_margin_top(g, 16);
        static const char* net_names[] = {"User (SLIRP)","TAP","Bridge","Socket",nullptr};
        net_combo = gtk_drop_down_new_from_strings(net_names);
        gtk_widget_set_hexpand(net_combo, TRUE);
        gtk_grid_attach(GTK_GRID(g), make_label("Network:"), 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(g), net_combo, 1, 0, 1, 1);
        add("Network", g);
    }
    
    // ---- Hardware page ----
    {
        GtkWidget* g = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(g), 10);
        gtk_grid_set_column_spacing(GTK_GRID(g), 12);
        gtk_widget_set_margin_start(g, 20); gtk_widget_set_margin_end(g, 20);
        gtk_widget_set_margin_top(g, 16);
        int r = 0;
        
        // Scan /usr/bin for qemu binaries
        std::vector<std::string> bins = CommandBuilder::listInstalledBinaries(VMMode::System);
        std::vector<const char*> bin_cstr;
        for (auto& b : bins) bin_cstr.push_back(b.c_str());
        bin_cstr.push_back("Custom...");
        bin_cstr.push_back(nullptr);
        bin_combo = gtk_drop_down_new_from_strings(bin_cstr.data());
        gtk_widget_set_hexpand(bin_combo, TRUE);
        gtk_grid_attach(GTK_GRID(g), make_label("QEMU Binary:"), 0, r, 1, 1);
        gtk_grid_attach(GTK_GRID(g), bin_combo, 1, r, 1, 1); r++;
        
        custom_bin_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(custom_bin_entry), "qemu-system-...");
        gtk_widget_set_sensitive(custom_bin_entry, FALSE);
        gtk_widget_set_hexpand(custom_bin_entry, TRUE);
        gtk_grid_attach(GTK_GRID(g), custom_bin_entry, 1, r, 1, 1); r++;
        
        bin_hint_lbl = gtk_label_new("");
        gtk_widget_add_css_class(bin_hint_lbl, "dim-label");
        gtk_label_set_xalign(GTK_LABEL(bin_hint_lbl), 0);
        gtk_grid_attach(GTK_GRID(g), bin_hint_lbl, 0, r, 2, 1); r++;
        
        g_object_set_data(G_OBJECT(bin_combo), "custom_idx", GINT_TO_POINTER((int)bins.size()));
        g_object_set_data(G_OBJECT(bin_combo), "entry", custom_bin_entry);
        g_object_set_data(G_OBJECT(bin_combo), "hint", bin_hint_lbl);
        g_signal_connect(bin_combo, "notify::selected",
            G_CALLBACK(+[](GtkDropDown* dd, GParamSpec*, gpointer) {
                int custom_idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dd), "custom_idx"));
                GtkWidget* ent = (GtkWidget*)g_object_get_data(G_OBJECT(dd), "entry");
                bool is_custom = (int)gtk_drop_down_get_selected(dd) == custom_idx;
                gtk_widget_set_sensitive(ent, is_custom);
                if (!is_custom) {
                    GtkWidget* hint = (GtkWidget*)g_object_get_data(G_OBJECT(dd), "hint");
                    gtk_label_set_text(GTK_LABEL(hint), "");
                }
            }), nullptr);
        
        gtk_grid_attach(GTK_GRID(g), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, r, 2, 1); r++;
        
        static const char* accel_names[] = {
            "TCG  (software emulation - works everywhere)",
            "KVM  (hardware acceleration - requires /dev/kvm)",
            "KVM + LBT  (LoongArch silicon binary translation)",
            "WHPX  (Windows Hypervisor Platform)",
            "HVF   (Apple Hypervisor Framework - macOS)",
            "NVMM  (NetBSD Virtual Machine Monitor)",
            "Xen   (Xen hypervisor)",
            "Nitro (AWS Nitro Enclave - QEMU 11)",
            "MSHV  (Microsoft Hyper-V - QEMU 11)",
            nullptr
        };
        accel_combo = gtk_drop_down_new_from_strings(accel_names);
        gtk_widget_set_hexpand(accel_combo, TRUE);
        gtk_grid_attach(GTK_GRID(g), make_label("Accelerator:"), 0, r, 1, 1);
        gtk_grid_attach(GTK_GRID(g), accel_combo, 1, r, 1, 1); r++;
        
        lbt_note_lbl = gtk_label_new("LoongArch LBT: silicon-level x86/MIPS/ARM translation.\nRequires LoongArch hardware with LBT extension.");
        gtk_label_set_wrap(GTK_LABEL(lbt_note_lbl), TRUE);
        gtk_widget_add_css_class(lbt_note_lbl, "dim-label");
        gtk_label_set_xalign(GTK_LABEL(lbt_note_lbl), 0);
        gtk_widget_set_visible(lbt_note_lbl, FALSE);
        gtk_grid_attach(GTK_GRID(g), lbt_note_lbl, 0, r, 2, 1); r++;
        
        g_signal_connect(accel_combo, "notify::selected",
            G_CALLBACK(+[](GtkDropDown* dd, GParamSpec*, gpointer ud) {
                gtk_widget_set_visible((GtkWidget*)ud, gtk_drop_down_get_selected(dd) == 2);
            }), lbt_note_lbl);
        
        gtk_grid_attach(GTK_GRID(g), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, r, 2, 1); r++;
        
        extra_args_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(extra_args_entry), "-device virtio-rng-pci -no-reboot");
        gtk_widget_set_hexpand(extra_args_entry, TRUE);
        gtk_grid_attach(GTK_GRID(g), make_label("Extra Args:"), 0, r, 1, 1);
        gtk_grid_attach(GTK_GRID(g), extra_args_entry, 1, r, 1, 1); r++;
        
        save_script_check = gtk_check_button_new_with_label("Save start.sh to VM folder");
        gtk_grid_attach(GTK_GRID(g), save_script_check, 0, r, 2, 1); r++;

        gtk_grid_attach(GTK_GRID(g), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, r, 2, 1); r++;

        // QEMU 11 security controls
        kvm_cet_check = gtk_check_button_new_with_label("KVM CET (Intel Control-flow Enforcement Technology, QEMU 11)");
        gtk_grid_attach(GTK_GRID(g), kvm_cet_check, 0, r, 2, 1); r++;

        gtk_grid_attach(GTK_GRID(g), make_label("Confidential VM:"), 0, r, 1, 1);
        static const char* cvm_names[] = {"None","SEV-SNP (AMD, QEMU 11 KVM)","TDX (Intel, QEMU 11 KVM)",nullptr};
        conf_vm_combo = gtk_drop_down_new_from_strings(cvm_names);
        gtk_widget_set_hexpand(conf_vm_combo, TRUE);
        gtk_grid_attach(GTK_GRID(g), conf_vm_combo, 1, r, 1, 1); r++;

        scsi_mq_check = gtk_check_button_new_with_label("VirtIO-SCSI Multiqueue (one I/O thread per vCPU, QEMU 10)");
        gtk_grid_attach(GTK_GRID(g), scsi_mq_check, 0, r, 2, 1); r++;

        riscv_iommu_check = gtk_check_button_new_with_label("RISC-V IOMMU sys device (riscv-iommu-sys, QEMU 11)");
        gtk_grid_attach(GTK_GRID(g), riscv_iommu_check, 0, r, 2, 1); r++;

        // ── QEMU 10.2: io_uring + CPR-exec migration + 9pfs ────────────
        io_uring_check = gtk_check_button_new_with_label("io_uring main loop (Linux 5.1+, QEMU 10.2 perf boost)");
        gtk_grid_attach(GTK_GRID(g), io_uring_check, 0, r, 2, 1); r++;

        static const char* mig_names[] = {"None","CPR-exec (live update, QEMU 10.2)","SaveVM",nullptr};
        migration_combo = gtk_drop_down_new_from_strings(mig_names);
        gtk_widget_set_hexpand(migration_combo, TRUE);
        {auto* lm = make_label("Migration mode:"); gtk_grid_attach(GTK_GRID(g),lm,0,r,1,1); gtk_grid_attach(GTK_GRID(g),migration_combo,1,r,1,1); r++;}

        virtfs_path_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(virtfs_path_entry), "/path/to/share  (blank = disabled)");
        gtk_widget_set_hexpand(virtfs_path_entry, TRUE);
        {auto* lp = make_label("9pfs VirtFS host path:"); gtk_grid_attach(GTK_GRID(g),lp,0,r,1,1); gtk_grid_attach(GTK_GRID(g),virtfs_path_entry,1,r,1,1); r++;}

        virtfs_tag_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(virtfs_tag_entry), "host_share");
        gtk_widget_set_hexpand(virtfs_tag_entry, TRUE);
        {auto* lt = make_label("9pfs mount tag:"); gtk_grid_attach(GTK_GRID(g),lt,0,r,1,1); gtk_grid_attach(GTK_GRID(g),virtfs_tag_entry,1,r,1,1); r++;}

        add("Hardware", g);
    }
    
    // ---- Preview page ----
    {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_widget_set_margin_start(box, 16); gtk_widget_set_margin_end(box, 16);
        gtk_widget_set_margin_top(box, 12);
        GtkWidget* lbl = gtk_label_new("Generated QEMU Command:");
        gtk_label_set_xalign(GTK_LABEL(lbl), 0);
        GtkWidget* scroll = gtk_scrolled_window_new();
        gtk_widget_set_vexpand(scroll, TRUE);
        cmd_preview = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(cmd_preview), FALSE);
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(cmd_preview), TRUE);
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(cmd_preview), GTK_WRAP_WORD_CHAR);
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), cmd_preview);
        gtk_box_append(GTK_BOX(box), lbl);
        gtk_box_append(GTK_BOX(box), scroll);
        add("Preview", box);
    }
    
    gtk_box_append(GTK_BOX(vbox), notebook);
    
    // Buttons
    GtkWidget* btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(btns, 16); gtk_widget_set_margin_end(btns, 16);
    gtk_widget_set_margin_top(btns, 8); gtk_widget_set_margin_bottom(btns, 12);
    GtkWidget* sp = gtk_label_new(""); gtk_widget_set_hexpand(sp, TRUE);
    GtkWidget* cancel = gtk_button_new_with_label("Cancel");
    GtkWidget* save = gtk_button_new_with_label("Save Changes");
    gtk_widget_add_css_class(save, "suggested-action");
    g_signal_connect(cancel, "clicked", G_CALLBACK(onCancelClicked), this);
    g_signal_connect(save, "clicked", G_CALLBACK(onSaveClicked), this);
    gtk_box_append(GTK_BOX(btns), sp);
    gtk_box_append(GTK_BOX(btns), cancel);
    gtk_box_append(GTK_BOX(btns), save);
    gtk_box_append(GTK_BOX(vbox), btns);
    
}

void VMSettingsDialog::updatePreview() {
    VMConfig c = collectConfig();
    std::string cmd = CommandBuilder::formatCommand(c);
    GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(cmd_preview));
    gtk_text_buffer_set_text(buf, cmd.c_str(), -1);
}

VMConfig VMSettingsDialog::collectConfig() {
    VMConfig c = vm_ref; // Start from existing config
    c.name = gtk_editable_get_text(GTK_EDITABLE(name_entry));
    c.description = gtk_editable_get_text(GTK_EDITABLE(desc_entry));
    c.arch = (VMArch)gtk_drop_down_get_selected(GTK_DROP_DOWN(arch_combo));
    c.mode = gtk_drop_down_get_selected(GTK_DROP_DOWN(mode_combo)) == 1 ? VMMode::UserMode : VMMode::System;
    int mi = gtk_drop_down_get_selected(GTK_DROP_DOWN(mach_combo));
    const MachineType mt[] = {MachineType::q35,MachineType::pc,MachineType::virt,MachineType::microvm,MachineType::sbsa_ref,MachineType::virt_acpi,MachineType::x86_64_microvm,MachineType::nitro_enclave,MachineType::amd_versal2_virt,MachineType::custom};
    c.machine = mt[mi < 5 ? mi : 0];
    if (c.machine == MachineType::custom)
        c.machine_custom = gtk_editable_get_text(GTK_EDITABLE(mach_custom));
    c.cpu_model = gtk_editable_get_text(GTK_EDITABLE(cpu_entry));
    c.sockets = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(sockets_spin));
    c.cores = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(cores_spin));
    c.threads = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(threads_spin));
    c.ram_mb = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(ram_spin));
    c.ballooning = gtk_check_button_get_active(GTK_CHECK_BUTTON(balloon_check));
    c.disk_size_gb = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(disk_size_spin));
    c.disk_format = (DiskFormat)gtk_drop_down_get_selected(GTK_DROP_DOWN(disk_fmt_combo));
    c.iso_path = gtk_editable_get_text(GTK_EDITABLE(iso_entry));
    c.firmware = gtk_check_button_get_active(GTK_CHECK_BUTTON(uefi_check)) ? Firmware::OVMF : Firmware::SeaBIOS;
    c.boot_menu = gtk_check_button_get_active(GTK_CHECK_BUTTON(boot_menu_check));
    const char* bo[] = {"cd","dc","c","d"};
    int boi = gtk_drop_down_get_selected(GTK_DROP_DOWN(boot_combo));
    c.boot_order = bo[boi < 4 ? boi : 0];
    const DisplayType dt[] = {DisplayType::SDL,DisplayType::GTK,DisplayType::SPICE,DisplayType::VNC,DisplayType::EGL,DisplayType::Headless};
    c.display = dt[gtk_drop_down_get_selected(GTK_DROP_DOWN(display_combo))];
    c.gpu = (GPUType)gtk_drop_down_get_selected(GTK_DROP_DOWN(gpu_combo));
    c.audio = (AudioType)gtk_drop_down_get_selected(GTK_DROP_DOWN(audio_combo));
    c.net_mode = (NetworkMode)gtk_drop_down_get_selected(GTK_DROP_DOWN(net_combo));
    
    int custom_idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(bin_combo), "custom_idx"));
    int sel = (int)gtk_drop_down_get_selected(GTK_DROP_DOWN(bin_combo));
    if (sel == custom_idx) {
        c.binary_mode = BinaryMode::Custom;
        c.custom_binary = gtk_editable_get_text(GTK_EDITABLE(custom_bin_entry));
    } else {
        c.binary_mode = BinaryMode::Auto;
        c.custom_binary = "";
    }
    
    const Accelerator ac[] = {
        Accelerator::TCG, Accelerator::KVM, Accelerator::KVM_LBT,
        Accelerator::WHPX, Accelerator::HVF, Accelerator::NVMM,
        Accelerator::Xen, Accelerator::Nitro, Accelerator::MSHV
    };
    int ai = gtk_drop_down_get_selected(GTK_DROP_DOWN(accel_combo));
    c.accel = ac[ai < 9 ? ai : 0];
    
    c.extra_args = gtk_editable_get_text(GTK_EDITABLE(extra_args_entry));
    c.save_script_to_vm_folder = gtk_check_button_get_active(GTK_CHECK_BUTTON(save_script_check));
    c.kvm_cet = gtk_check_button_get_active(GTK_CHECK_BUTTON(kvm_cet_check));
    c.conf_vm = (ConfidentialVM)gtk_drop_down_get_selected(GTK_DROP_DOWN(conf_vm_combo));
    c.scsi_multiqueue = gtk_check_button_get_active(GTK_CHECK_BUTTON(scsi_mq_check));
    c.riscv_iommu = gtk_check_button_get_active(GTK_CHECK_BUTTON(riscv_iommu_check));
    // QEMU 10.2 new fields
    c.io_uring_loop   = gtk_check_button_get_active(GTK_CHECK_BUTTON(io_uring_check));
    c.migration_mode  = (MigrationMode)gtk_drop_down_get_selected(GTK_DROP_DOWN(migration_combo));
    const char* vp = gtk_editable_get_text(GTK_EDITABLE(virtfs_path_entry));
    c.virtfs_path = vp ? vp : "";
    const char* vt = gtk_editable_get_text(GTK_EDITABLE(virtfs_tag_entry));
    c.virtfs_mount_tag = (vt && *vt) ? vt : "host_share";
    c.virtfs_driver = c.virtfs_path.empty() ? VirtFSDriver::None : VirtFSDriver::Local_9P;

    // Preserve vm_dir and disk_path
    c.vm_dir = vm_ref.vm_dir;
    const char* exts[] = {"qcow2","raw","vmdk","vdi"};
    int fi = (int)c.disk_format;
    c.disk_path = c.vm_dir + "/disk." + exts[fi < 4 ? fi : 0];
    
    // Preserve runtime state
    c.status = vm_ref.status;
    c.last_started = vm_ref.last_started;
    
    return c;
}

void VMSettingsDialog::onBrowseISO(GtkButton*, gpointer d) {
    VMSettingsDialog* s = (VMSettingsDialog*)d;
#if GTK_CHECK_VERSION(4, 10, 0)
    GtkFileDialog* dlg = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dlg, "Select ISO Image");
    GListStore* fs = g_list_store_new(GTK_TYPE_FILE_FILTER);
    GtkFileFilter* f = gtk_file_filter_new();
    gtk_file_filter_add_pattern(f, "*.iso");
    gtk_file_filter_set_name(f, "ISO Images");
    g_list_store_append(fs, f); g_object_unref(f);
    gtk_file_dialog_set_filters(dlg, G_LIST_MODEL(fs)); g_object_unref(fs);
    static auto cb = [](GObject* src, GAsyncResult* res, gpointer ud) {
        GtkEntry* entry = (GtkEntry*)ud;
        GFile* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(src), res, nullptr);
        if (file) {
            char* p = g_file_get_path(file);
            if (p) { gtk_editable_set_text(GTK_EDITABLE(entry), p); g_free(p); }
            g_object_unref(file);
        }
    };
    gtk_file_dialog_open(dlg, GTK_WINDOW(s->dialog), nullptr, (GAsyncReadyCallback)cb, s->iso_entry);
    g_object_unref(dlg);
#else
    compat_open_file_chooser(GTK_WINDOW(s->dialog), "Select ISO Image",
        GTK_ENTRY(s->iso_entry), "*.iso", "ISO Images");
#endif
}

void VMSettingsDialog::onSaveClicked(GtkButton*, gpointer d) {
    VMSettingsDialog* s = (VMSettingsDialog*)d;
    VMConfig c = s->collectConfig();
    if (c.name.empty()) {
#if GTK_CHECK_VERSION(4, 10, 0)
        GtkAlertDialog* ad = gtk_alert_dialog_new("Please enter a VM name.");
        gtk_alert_dialog_show(ad, GTK_WINDOW(s->dialog));
        g_object_unref(ad);
#else
        compat_show_alert(GTK_WINDOW(s->dialog), "Please enter a VM name.");
#endif
        return;
    }
    if (c.binary_mode == BinaryMode::Custom) {
        std::string err = CommandBuilder::validateCustomBinary(c.custom_binary);
        if (!err.empty()) {
#if GTK_CHECK_VERSION(4, 10, 0)
            GtkAlertDialog* ad = gtk_alert_dialog_new(("Binary error: " + err).c_str());
            gtk_alert_dialog_show(ad, GTK_WINDOW(s->dialog));
            g_object_unref(ad);
#else
            compat_show_alert(GTK_WINDOW(s->dialog), ("Binary error: " + err).c_str());
#endif
            return;
        }
    }
    VMConfigIO::save(c);
    if (c.save_script_to_vm_folder)
        CommandBuilder::writeStartScript(c);
    if (s->on_save_cb) s->on_save_cb(c);
    gtk_window_destroy(GTK_WINDOW(s->dialog));
    delete s;
}

void VMSettingsDialog::onCancelClicked(GtkButton*, gpointer d) {
    VMSettingsDialog* s = (VMSettingsDialog*)d;
    gtk_window_destroy(GTK_WINDOW(s->dialog));
    delete s;
}

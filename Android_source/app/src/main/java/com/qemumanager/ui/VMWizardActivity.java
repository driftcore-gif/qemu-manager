package com.qemumanager.ui;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.widget.*;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import com.qemumanager.R;
import com.qemumanager.model.VMConfig;
import com.qemumanager.util.CommandBuilder;
import com.qemumanager.util.DiskManager;
import com.qemumanager.util.VMConfigIO;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class VMWizardActivity extends AppCompatActivity {

    // ── Views ──────────────────────────────────────────────────────────
    private EditText  nameEntry, descEntry, machCustomEntry;
    private Spinner   archSpinner, modeSpinner, machSpinner;
    private Spinner   cpuSpinner, gpuSpinner, audioSpinner, accelSpinner;
    private EditText  cpuFlagsEntry;
    private SeekBar   socketsSb, coresSb, threadsSb;
    private TextView  socketsVal, coresVal, threadsVal;
    private SeekBar   ramSb;
    private TextView  ramVal;
    private CheckBox  balloonCheck;

    // Disk section
    private RadioGroup  diskModeGroup;
    private RadioButton radioDiskNew, radioDiskExisting, radioDiskNone;
    private LinearLayout diskNewPanel, diskExistingPanel;
    private SeekBar      diskSizeSb;
    private TextView     diskSizeVal;
    private Spinner      diskFmtSpinner;
    private CheckBox     preallocCheck;
    private TextView     diskStatusText;
    private Button       btnBrowseDisk;
    private TextView     existingDiskPath;

    // ISO
    private EditText  isoEntry;
    private Button    btnBrowseIso;

    // Boot / Display / Network / Hardware
    private CheckBox  uefiCheck, bootMenuCheck;
    private Spinner   bootSpinner, displaySpinner, netSpinner;
    private EditText  extraArgsEntry;
    private TextView  lbtNoteText;
    private CheckBox  saveScriptCheck;
    private TextView  cmdPreview;

    // State
    private String vmBaseDir;
    private String selectedExistingDisk = "";
    private int[]  ramSteps = {128,256,512,1024,2048,4096,8192,16384,32768,65536};
    // Disk size slider: 1–2000 GB
    private static final int DISK_MAX_GB = 2000;

    // File picker
    private ActivityResultLauncher<Intent> diskPicker;
    private ActivityResultLauncher<Intent> isoPicker;

    private enum ArchGroup { X86, I386, AARCH64, ARM, RISCV64, RISCV32, MIPS, PPC, S390, LOONG, GENERIC }

    private ArchGroup archGroup(int pos) {
        switch (pos) {
            case 0:  return ArchGroup.X86;
            case 1:  return ArchGroup.I386;
            case 2:  return ArchGroup.AARCH64;
            case 3: case 4: return ArchGroup.ARM;
            case 5:  return ArchGroup.RISCV64;
            case 6:  return ArchGroup.RISCV32;
            case 7: case 8: case 9: case 10: case 11: case 12: return ArchGroup.MIPS;
            case 13: case 14: case 15: return ArchGroup.PPC;
            case 26: return ArchGroup.S390;
            case 25: return ArchGroup.LOONG;
            default: return ArchGroup.GENERIC;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            setContentView(R.layout.activity_vm_wizard);
            vmBaseDir = getIntent().getStringExtra("vm_base_dir");
            if (vmBaseDir == null) {
                File ext = getExternalFilesDir(null);
                if (ext == null) ext = getFilesDir();
                vmBaseDir = ext.getAbsolutePath() + "/VMs";
            }
            if (getSupportActionBar() != null)
                getSupportActionBar().setTitle("New Virtual Machine");

            registerPickers();
            bindViews();
            setupSeekBars();
            setupDiskSection();
            setupArchListener();
            triggerArchUpdate(0);

            Button btnCreate = findViewById(R.id.btn_create);
            Button btnCancel = findViewById(R.id.btn_cancel);
            if (btnCreate != null) btnCreate.setOnClickListener(v -> onCreateClicked());
            if (btnCancel != null) btnCancel.setOnClickListener(v -> finish());
        } catch (Exception e) {
            Toast.makeText(this, "Error: " + e.getMessage(), Toast.LENGTH_LONG).show();
            finish();
        }
    }

    // ── File pickers ─────────────────────────────────────────────────
    private void registerPickers() {
        diskPicker = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(), result -> {
                if (result.getResultCode() == RESULT_OK && result.getData() != null) {
                    Uri uri = result.getData().getData();
                    if (uri != null) {
                        String path = resolveUri(uri);
                        selectedExistingDisk = path != null ? path : uri.toString();
                        if (existingDiskPath != null)
                            existingDiskPath.setText(selectedExistingDisk);
                        if (diskStatusText != null)
                            inspectAndShowDisk(selectedExistingDisk);
                    }
                }
            });

        isoPicker = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(), result -> {
                if (result.getResultCode() == RESULT_OK && result.getData() != null) {
                    Uri uri = result.getData().getData();
                    if (uri != null) {
                        String path = resolveUri(uri);
                        if (isoEntry != null)
                            isoEntry.setText(path != null ? path : uri.toString());
                    }
                }
            });
    }

    private String resolveUri(Uri uri) {
        try {
            String scheme = uri.getScheme();
            if ("file".equals(scheme)) return uri.getPath();
            if ("content".equals(scheme)) {
                // Try path segment
                String path = uri.getPath();
                if (path != null && path.startsWith("/storage")) return path;
                // DocumentProvider path
                List<String> segs = uri.getPathSegments();
                if (segs != null && segs.size() >= 2) {
                    String last = segs.get(segs.size() - 1);
                    if (last.contains(":")) {
                        String[] parts = last.split(":");
                        if (parts.length == 2)
                            return "/storage/emulated/0/" + parts[1];
                    }
                }
            }
        } catch (Exception ignored) {}
        return null;
    }

    // ── Bind views ────────────────────────────────────────────────────
    private void bindViews() {
        nameEntry      = findViewById(R.id.et_name);
        descEntry      = findViewById(R.id.et_desc);
        machCustomEntry= findViewById(R.id.et_mach_custom);
        archSpinner    = findViewById(R.id.sp_arch);
        modeSpinner    = findViewById(R.id.sp_mode);
        machSpinner    = findViewById(R.id.sp_machine);
        cpuSpinner     = findViewById(R.id.sp_cpu);
        cpuFlagsEntry  = findViewById(R.id.et_cpu_flags);
        gpuSpinner     = findViewById(R.id.sp_gpu);
        audioSpinner   = findViewById(R.id.sp_audio);
        accelSpinner   = findViewById(R.id.sp_accel);
        socketsSb      = findViewById(R.id.sb_sockets);
        coresSb        = findViewById(R.id.sb_cores);
        threadsSb      = findViewById(R.id.sb_threads);
        socketsVal     = findViewById(R.id.tv_sockets);
        coresVal       = findViewById(R.id.tv_cores);
        threadsVal     = findViewById(R.id.tv_threads);
        ramSb          = findViewById(R.id.sb_ram);
        ramVal         = findViewById(R.id.tv_ram);
        balloonCheck   = findViewById(R.id.cb_balloon);

        // Disk
        diskModeGroup      = findViewById(R.id.rg_disk_mode);
        radioDiskNew       = findViewById(R.id.rb_disk_new);
        radioDiskExisting  = findViewById(R.id.rb_disk_existing);
        radioDiskNone      = findViewById(R.id.rb_disk_none);
        diskNewPanel       = findViewById(R.id.panel_disk_new);
        diskExistingPanel  = findViewById(R.id.panel_disk_existing);
        diskSizeSb         = findViewById(R.id.sb_disk_size);
        diskSizeVal        = findViewById(R.id.tv_disk_size);
        diskFmtSpinner     = findViewById(R.id.sp_disk_fmt);
        preallocCheck      = findViewById(R.id.cb_prealloc);
        diskStatusText     = findViewById(R.id.tv_disk_status);
        btnBrowseDisk      = findViewById(R.id.btn_browse_disk);
        existingDiskPath   = findViewById(R.id.tv_existing_disk_path);
        isoEntry           = findViewById(R.id.et_iso);
        btnBrowseIso       = findViewById(R.id.btn_browse_iso);

        bootSpinner    = findViewById(R.id.sp_boot);
        uefiCheck      = findViewById(R.id.cb_uefi);
        bootMenuCheck  = findViewById(R.id.cb_boot_menu);
        displaySpinner = findViewById(R.id.sp_display);
        netSpinner     = findViewById(R.id.sp_net);
        extraArgsEntry = findViewById(R.id.et_extra_args);
        lbtNoteText    = findViewById(R.id.tv_lbt_note);
        saveScriptCheck= findViewById(R.id.cb_save_script);
        cmdPreview     = findViewById(R.id.tv_cmd_preview);
    }

    // ── SeekBars ─────────────────────────────────────────────────────
    private void setupSeekBars() {
        setupSeekBar(socketsSb, socketsVal, 1, 8,  1, "Sockets: ");
        setupSeekBar(coresSb,   coresVal,   1, 64, 2, "Cores: ");
        setupSeekBar(threadsSb, threadsVal, 1, 8,  2, "Threads: ");
        setupRamSeekBar();
    }

    private void setupSeekBar(SeekBar sb, TextView tv, int min, int max, int def, String label) {
        if (sb == null || tv == null) return;
        sb.setMax(max - min);
        sb.setProgress(def - min);
        tv.setText(label + def);
        sb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar s, int p, boolean u) { tv.setText(label + (p + min)); updatePreview(); }
            public void onStartTrackingTouch(SeekBar s) {}
            public void onStopTrackingTouch(SeekBar s) {}
        });
    }

    private void setupRamSeekBar() {
        if (ramSb == null || ramVal == null) return;
        ramSb.setMax(ramSteps.length - 1);
        ramSb.setProgress(4);
        ramVal.setText("RAM: 2048 MB");
        ramSb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar s, int p, boolean u) {
                int mb = ramSteps[Math.min(p, ramSteps.length - 1)];
                ramVal.setText(mb >= 1024 ? "RAM: " + (mb/1024) + " GB" : "RAM: " + mb + " MB");
                updatePreview();
            }
            public void onStartTrackingTouch(SeekBar s) {}
            public void onStopTrackingTouch(SeekBar s) {}
        });
    }

    // ── Disk section ─────────────────────────────────────────────────
    private void setupDiskSection() {
        // Disk size slider: 1 GB to 2000 GB (step=1)
        if (diskSizeSb != null) {
            diskSizeSb.setMax(DISK_MAX_GB - 1);
            diskSizeSb.setProgress(19); // default 20 GB
            updateDiskSizeLabel(20);
            diskSizeSb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                public void onProgressChanged(SeekBar s, int p, boolean u) {
                    updateDiskSizeLabel(p + 1);
                    updatePreview();
                }
                public void onStartTrackingTouch(SeekBar s) {}
                public void onStopTrackingTouch(SeekBar s) {}
            });
        }

        // Disk mode radio
        if (diskModeGroup != null) {
            diskModeGroup.setOnCheckedChangeListener((g, id) -> onDiskModeChanged(id));
            onDiskModeChanged(R.id.rb_disk_new); // default: create new
        }

        // Browse existing disk
        if (btnBrowseDisk != null) {
            btnBrowseDisk.setOnClickListener(v -> {
                Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                i.setType("*/*");
                i.putExtra(Intent.EXTRA_MIME_TYPES,
                    new String[]{"application/octet-stream","*/*"});
                i.addCategory(Intent.CATEGORY_OPENABLE);
                diskPicker.launch(i);
            });
        }

        // Browse ISO
        if (btnBrowseIso != null) {
            btnBrowseIso.setOnClickListener(v -> {
                Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                i.setType("*/*");
                i.addCategory(Intent.CATEGORY_OPENABLE);
                isoPicker.launch(i);
            });
        }

        // Show qemu-img status
        if (diskStatusText != null) {
            boolean available = DiskManager.isQemuImgAvailable();
            diskStatusText.setText(available
                ? "✓ qemu-img found at " + DiskManager.resolveQemuImg()
                : "⚠ qemu-img not found — install QEMU in Termux/PRoot first");
            diskStatusText.setTextColor(available ? 0xFF4CAF50 : 0xFFFFC107);
        }
    }

    private void updateDiskSizeLabel(int gb) {
        if (diskSizeVal == null) return;
        if (gb >= 1024)
            diskSizeVal.setText("Disk: " + String.format("%.1f", gb/1024.0) + " TB");
        else
            diskSizeVal.setText("Disk: " + gb + " GB");
    }

    private void onDiskModeChanged(int radioId) {
        if (diskNewPanel == null || diskExistingPanel == null) return;
        if (radioId == R.id.rb_disk_new) {
            diskNewPanel.setVisibility(View.VISIBLE);
            diskExistingPanel.setVisibility(View.GONE);
        } else if (radioId == R.id.rb_disk_existing) {
            diskNewPanel.setVisibility(View.GONE);
            diskExistingPanel.setVisibility(View.VISIBLE);
        } else { // no disk
            diskNewPanel.setVisibility(View.GONE);
            diskExistingPanel.setVisibility(View.GONE);
        }
    }

    private void inspectAndShowDisk(String path) {
        new Thread(() -> {
            DiskManager.DiskInfo info = DiskManager.inspectDisk(path);
            runOnUiThread(() -> {
                if (diskStatusText == null) return;
                if (info.valid) {
                    diskStatusText.setText("Disk: " + info.format.toUpperCase()
                        + " · Virtual: " + DiskManager.humanSize(info.virtualSizeBytes)
                        + " · Actual: " + DiskManager.humanSize(info.actualSizeBytes));
                    diskStatusText.setTextColor(0xFF4CAF50);
                } else {
                    diskStatusText.setText("⚠ Not a valid disk image");
                    diskStatusText.setTextColor(0xFFFFC107);
                }
            });
        }).start();
    }

    // ── Arch-driven dropdowns ─────────────────────────────────────────
    private void setupArchListener() {
        if (archSpinner == null) return;
        archSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> p, View v, int pos, long id) { triggerArchUpdate(pos); }
            public void onNothingSelected(AdapterView<?> p) {}
        });
    }

    private void triggerArchUpdate(int pos) {
        ArchGroup g = archGroup(pos);
        populateSpinner(machSpinner,  machArr(g));
        populateSpinner(cpuSpinner,   cpuArr(g));
        populateSpinner(gpuSpinner,   gpuArr(g));
        populateSpinner(audioSpinner, audioArr(g));
        populateSpinner(accelSpinner, accelArr(g));

        if (machSpinner != null) {
            machSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                public void onItemSelected(AdapterView<?> p, View v, int i, long id) {
                    String s = (String) p.getItemAtPosition(i);
                    if (machCustomEntry != null)
                        machCustomEntry.setVisibility("custom".equals(s) ? View.VISIBLE : View.GONE);
                    updatePreview();
                }
                public void onNothingSelected(AdapterView<?> p) {}
            });
        }
        if (accelSpinner != null) {
            accelSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                public void onItemSelected(AdapterView<?> p, View v, int i, long id) {
                    String s = (String) p.getItemAtPosition(i);
                    if (lbtNoteText != null)
                        lbtNoteText.setVisibility(s != null && s.startsWith("KVM + LBT") ? View.VISIBLE : View.GONE);
                    updatePreview();
                }
                public void onNothingSelected(AdapterView<?> p) {}
            });
        }
    }

    private void populateSpinner(Spinner sp, int resId) {
        if (sp == null) return;
        String[] items = getResources().getStringArray(resId);
        ArrayAdapter<String> ad = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, items);
        ad.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        sp.setAdapter(ad);
    }

    private int machArr(ArchGroup g) {
        switch(g) { case X86: case I386: return R.array.machine_x86; case AARCH64: case ARM: return R.array.machine_arm; case RISCV64: case RISCV32: return R.array.machine_riscv; case MIPS: return R.array.machine_mips; case PPC: return R.array.machine_ppc; case S390: return R.array.machine_s390; case LOONG: return R.array.machine_loong; default: return R.array.machine_generic; }
    }
    private int cpuArr(ArchGroup g) {
        switch(g) { case X86: return R.array.cpu_x86_64; case I386: return R.array.cpu_i386; case AARCH64: return R.array.cpu_aarch64; case ARM: return R.array.cpu_arm; case RISCV64: return R.array.cpu_riscv64; case RISCV32: return R.array.cpu_riscv32; case MIPS: return R.array.cpu_mips; case PPC: return R.array.cpu_ppc; case S390: return R.array.cpu_s390; case LOONG: return R.array.cpu_loong; default: return R.array.cpu_generic; }
    }
    private int gpuArr(ArchGroup g) {
        switch(g) { case X86: case I386: return R.array.gpu_x86; case AARCH64: case ARM: return R.array.gpu_arm; default: return R.array.gpu_minimal; }
    }
    private int audioArr(ArchGroup g) {
        switch(g) { case X86: case I386: return R.array.audio_x86; case AARCH64: case ARM: return R.array.audio_arm; default: return R.array.audio_generic; }
    }
    private int accelArr(ArchGroup g) {
        switch(g) { case X86: case I386: return R.array.accel_x86; case AARCH64: case ARM: return R.array.accel_arm; default: return R.array.accel_generic; }
    }

    // ── Preview ──────────────────────────────────────────────────────
    private void updatePreview() {
        try {
            VMConfig c = collectConfig();
            if (cmdPreview != null) cmdPreview.setText(CommandBuilder.formatCommand(c));
        } catch (Exception ignored) {}
    }

    // ── Collect config ────────────────────────────────────────────────
    private VMConfig collectConfig() {
        VMConfig c = new VMConfig();

        c.name        = nameEntry != null ? nameEntry.getText().toString().trim() : "";
        c.description = descEntry != null ? descEntry.getText().toString().trim() : "";

        int archPos = archSpinner != null ? archSpinner.getSelectedItemPosition() : 0;
        VMConfig.VMArch[] archs = VMConfig.VMArch.values();
        c.arch = archs[Math.min(archPos, archs.length-1)];
        c.mode = modeSpinner != null && modeSpinner.getSelectedItemPosition() == 1
            ? VMConfig.VMMode.UserMode : VMConfig.VMMode.System;

        // Machine
        if (machSpinner != null) {
            String m = (String) machSpinner.getSelectedItem();
            if (m == null) m = "";
            if      (m.startsWith("q35"))         c.machine = VMConfig.MachineType.q35;
            else if (m.startsWith("pc"))          c.machine = VMConfig.MachineType.pc;
            else if (m.startsWith("virt-acpi"))   c.machine = VMConfig.MachineType.virt_acpi;
            else if (m.startsWith("virt"))        c.machine = VMConfig.MachineType.virt;
            else if (m.startsWith("microvm"))     c.machine = VMConfig.MachineType.microvm;
            else if (m.startsWith("x86-microvm")) c.machine = VMConfig.MachineType.x86_64_microvm;
            else if (m.startsWith("nitro"))       c.machine = VMConfig.MachineType.nitro_enclave;
            else if (m.startsWith("sbsa"))        c.machine = VMConfig.MachineType.sbsa_ref;
            else                                  c.machine = VMConfig.MachineType.custom;
            if (c.machine == VMConfig.MachineType.custom && machCustomEntry != null)
                c.machineCustom = machCustomEntry.getText().toString().trim();
        }

        // CPU
        if (cpuSpinner != null) {
            String raw = (String) cpuSpinner.getSelectedItem();
            if (raw == null) raw = "max";
            int p = raw.indexOf(" (");
            c.cpuModel = p >= 0 ? raw.substring(0, p).trim() : raw.trim();
        }
        c.cpuFlags = cpuFlagsEntry != null ? cpuFlagsEntry.getText().toString().trim() : "";

        c.sockets = socketsSb != null ? socketsSb.getProgress() + 1 : 1;
        c.cores   = coresSb   != null ? coresSb.getProgress()   + 1 : 2;
        c.threads = threadsSb != null ? threadsSb.getProgress() + 1 : 2;

        int ri = ramSb != null ? Math.min(ramSb.getProgress(), ramSteps.length-1) : 4;
        c.ramMb = ramSteps[ri];

        // ── Disk mode ──
        int diskMode = diskModeGroup != null ? diskModeGroup.getCheckedRadioButtonId() : R.id.rb_disk_new;
        if (diskMode == R.id.rb_disk_existing) {
            c.diskPath   = selectedExistingDisk;
            c.diskSizeGb = 0;
        } else if (diskMode == R.id.rb_disk_none) {
            c.diskPath   = "";
            c.diskSizeGb = 0;
        } else {
            // Create new — path set later during onCreateClicked
            c.diskSizeGb = diskSizeSb != null ? diskSizeSb.getProgress() + 1 : 20;
            c.diskPath   = ""; // will be set after disk creation
        }

        // Disk format
        if (diskFmtSpinner != null) {
            String[] fmtMap = {"qcow2","raw","vmdk","vdi"};
            int fi = diskFmtSpinner.getSelectedItemPosition();
            try { c.diskFormat = VMConfig.DiskFormat.valueOf(fmtMap[fi < 4 ? fi : 0]); } catch (Exception ignored) {}
        }

        c.isoPath  = isoEntry != null ? isoEntry.getText().toString().trim() : "";
        c.uefi     = uefiCheck     != null && uefiCheck.isChecked();
        c.bootMenu = bootMenuCheck != null && bootMenuCheck.isChecked();
        String[] bos = {"cd","dc","c","d"};
        c.bootOrder = bos[bootSpinner != null ? Math.min(bootSpinner.getSelectedItemPosition(),3):0];

        if (displaySpinner != null) {
            String[] dm = {"SDL","GTK","SPICE","VNC","EGL","Headless"};
            try { c.display = VMConfig.DisplayType.valueOf(dm[Math.min(displaySpinner.getSelectedItemPosition(),5)]); } catch(Exception ignored){}
        }

        if (gpuSpinner != null) {
            String g = (String) gpuSpinner.getSelectedItem(); if (g==null) g="";
            if      (g.contains("GL"))          c.gpu = VMConfig.GPUType.VirtIO_GPU_GL;
            else if (g.contains("Rutabaga"))    c.gpu = VMConfig.GPUType.VirtIO_GPU_Rutabaga;
            else if (g.contains("NativeCtx"))   c.gpu = VMConfig.GPUType.VirtIO_GPU_NativeCtx;
            else if (g.contains("VirtIO"))      c.gpu = VMConfig.GPUType.VirtIO_GPU;
            else if (g.contains("QXL"))         c.gpu = VMConfig.GPUType.QXL;
            else if (g.contains("VMware"))      c.gpu = VMConfig.GPUType.VMwareSVGA;
            else if (g.contains("Cirrus"))      c.gpu = VMConfig.GPUType.Cirrus;
            else if (g.contains("ramfb"))       c.gpu = VMConfig.GPUType.ramfb;
            else if (g.contains("None"))        c.gpu = VMConfig.GPUType.None;
            else                                c.gpu = VMConfig.GPUType.VGA;
        }

        if (audioSpinner != null) {
            String a = (String) audioSpinner.getSelectedItem(); if (a==null) a="";
            if      (a.contains("Intel HDA")) c.audio = VMConfig.AudioType.IntelHDA;
            else if (a.contains("AC97"))      c.audio = VMConfig.AudioType.AC97;
            else if (a.contains("SB16"))      c.audio = VMConfig.AudioType.SB16;
            else if (a.contains("VirtIO"))    c.audio = VMConfig.AudioType.VirtIO_Sound;
            else                              c.audio = VMConfig.AudioType.None;
        }

        String[] nm = {"User","TAP","Bridge","Socket"};
        try { c.net = VMConfig.NetworkMode.valueOf(nm[Math.min(netSpinner != null ? netSpinner.getSelectedItemPosition():0,3)]); } catch(Exception ignored){}

        if (accelSpinner != null) {
            String a = (String) accelSpinner.getSelectedItem(); if (a==null) a="";
            if      (a.startsWith("KVM + LBT")) c.accel = VMConfig.Accelerator.KVM_LBT;
            else if (a.startsWith("KVM"))        c.accel = VMConfig.Accelerator.KVM;
            else if (a.startsWith("WHPX"))       c.accel = VMConfig.Accelerator.WHPX;
            else if (a.startsWith("HVF"))        c.accel = VMConfig.Accelerator.HVF;
            else if (a.startsWith("Nitro"))      c.accel = VMConfig.Accelerator.Nitro;
            else if (a.startsWith("MSHV"))       c.accel = VMConfig.Accelerator.MSHV;
            else                                 c.accel = VMConfig.Accelerator.TCG;
        }

        c.extraArgs = extraArgsEntry != null ? extraArgsEntry.getText().toString().trim() : "";
        c.saveScriptToFolder = saveScriptCheck != null && saveScriptCheck.isChecked();
        return c;
    }

    // ── Create VM ────────────────────────────────────────────────────
    private void onCreateClicked() {
        try {
            VMConfig c = collectConfig();
            if (c.name.isEmpty()) {
                if (nameEntry != null) nameEntry.setError("VM Name is required");
                return;
            }

            File dir = new File(vmBaseDir, c.name);
            if (!dir.exists() && !dir.mkdirs()) {
                Toast.makeText(this, "Failed to create VM folder", Toast.LENGTH_SHORT).show();
                return;
            }
            c.vmDir = dir.getAbsolutePath();

            int diskMode = diskModeGroup != null ? diskModeGroup.getCheckedRadioButtonId() : R.id.rb_disk_new;

            if (diskMode == R.id.rb_disk_new) {
                // Need to create a disk first
                if (!DiskManager.isQemuImgAvailable()) {
                    new AlertDialog.Builder(this)
                        .setTitle("qemu-img not found")
                        .setMessage("Install QEMU in Termux or PRoot to create disk images.\n\nCreate VM without a disk image (add disk manually later)?")
                        .setPositiveButton("Create anyway", (d, w) -> {
                            c.diskPath = "";
                            finalizeVM(c);
                        })
                        .setNegativeButton("Cancel", null)
                        .show();
                    return;
                }

                String[] fmtMap = {"qcow2","raw","vmdk","vdi"};
                String fmt = fmtMap[Math.min(c.diskFormat.ordinal(), 3)];
                String diskPath = c.vmDir + "/disk." + fmt;
                long sizeGb = c.diskSizeGb > 0 ? c.diskSizeGb : 20;
                boolean prealloc = preallocCheck != null && preallocCheck.isChecked();

                ProgressDialog pd = new ProgressDialog(this);
                pd.setTitle("Creating disk image");
                pd.setMessage("Allocating " + sizeGb + " GB " + fmt.toUpperCase() + "…");
                pd.setProgressStyle(ProgressDialog.STYLE_SPINNER);
                pd.setCancelable(false);
                pd.show();

                DiskManager.createDiskAsync(diskPath, sizeGb, fmt, prealloc,
                    new DiskManager.DiskCreateCallback() {
                        public void onSuccess(String path) {
                            pd.dismiss();
                            c.diskPath = path;
                            finalizeVM(c);
                        }
                        public void onError(String msg) {
                            pd.dismiss();
                            new AlertDialog.Builder(VMWizardActivity.this)
                                .setTitle("Disk creation failed")
                                .setMessage(msg + "\n\nCreate VM without a disk?")
                                .setPositiveButton("Create anyway", (d, w) -> {
                                    c.diskPath = "";
                                    finalizeVM(c);
                                })
                                .setNegativeButton("Cancel", null)
                                .show();
                        }
                        public void onProgress(String msg) {
                            pd.setMessage(msg);
                        }
                    });
            } else {
                finalizeVM(c);
            }
        } catch (Exception e) {
            Toast.makeText(this, "Error: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }

    private void finalizeVM(VMConfig c) {
        String err = VMConfigIO.saveWithError(c);
        if (err != null) {
            Toast.makeText(this, "Save failed: " + err, Toast.LENGTH_LONG).show();
            return;
        }
        if (c.saveScriptToFolder) VMConfigIO.writeStartScript(c);
        setResult(RESULT_OK);
        Toast.makeText(this, "VM created: " + c.name
            + (c.diskPath.isEmpty() ? "" : "\nDisk: " + c.diskPath), Toast.LENGTH_LONG).show();
        finish();
    }
}

package com.qemumanager.ui;

import android.content.res.TypedArray;
import android.graphics.Color;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.widget.*;
import androidx.appcompat.app.AppCompatActivity;
import com.qemumanager.R;
import com.qemumanager.model.VMConfig;
import com.qemumanager.util.CommandBuilder;
import com.qemumanager.util.VMConfigIO;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class VMWizardActivity extends AppCompatActivity {

    private EditText  nameEntry, descEntry, machCustomEntry;
    private Spinner   archSpinner, modeSpinner, machSpinner;
    private Spinner   cpuSpinner, gpuSpinner, audioSpinner, accelSpinner;
    private EditText  cpuFlagsEntry;
    private SeekBar   socketsSb, coresSb, threadsSb;
    private TextView  socketsVal, coresVal, threadsVal;
    private SeekBar   ramSb;
    private TextView  ramVal;
    private CheckBox  balloonCheck;
    private EditText  diskSizeEntry, isoEntry;
    private Spinner   diskFmtSpinner;
    private CheckBox  uefiCheck, bootMenuCheck;
    private Spinner   bootSpinner, displaySpinner, netSpinner;
    private EditText  extraArgsEntry;
    private TextView  lbtNoteText;
    private CheckBox  saveScriptCheck;
    private TextView  cmdPreview;

    private String vmBaseDir;
    private int[] ramSteps = {128,256,512,1024,2048,4096,8192,16384,32768,65536};

    // ── Arch group helpers ─────────────────────────────────────────────
    private enum ArchGroup { X86, I386, AARCH64, ARM, RISCV64, RISCV32, MIPS, PPC, S390, LOONG, GENERIC }

    private ArchGroup archGroup(int archPos) {
        switch (archPos) {
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

            bindViews();
            setupSeekBars();
            setupArchListener();     // drive all arch-specific dropdowns
            triggerArchUpdate(0);    // initialise with x86_64

            Button btnCreate = findViewById(R.id.btn_create);
            Button btnCancel = findViewById(R.id.btn_cancel);
            if (btnCreate != null) btnCreate.setOnClickListener(v -> onCreateClicked());
            if (btnCancel != null) btnCancel.setOnClickListener(v -> finish());
        } catch (Exception e) {
            Toast.makeText(this, "Error: " + e.getMessage(), Toast.LENGTH_LONG).show();
            e.printStackTrace();
            finish();
        }
    }

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
        diskSizeEntry  = findViewById(R.id.et_disk_size);
        isoEntry       = findViewById(R.id.et_iso);
        diskFmtSpinner = findViewById(R.id.sp_disk_fmt);
        uefiCheck      = findViewById(R.id.cb_uefi);
        bootMenuCheck  = findViewById(R.id.cb_boot_menu);
        bootSpinner    = findViewById(R.id.sp_boot);
        displaySpinner = findViewById(R.id.sp_display);
        netSpinner     = findViewById(R.id.sp_net);
        extraArgsEntry = findViewById(R.id.et_extra_args);
        lbtNoteText    = findViewById(R.id.tv_lbt_note);
        saveScriptCheck= findViewById(R.id.cb_save_script);
        cmdPreview     = findViewById(R.id.tv_cmd_preview);
    }

    // ── SeekBars ──────────────────────────────────────────────────────
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
        ramSb.setProgress(4); // 2048
        ramVal.setText("RAM: 2048 MB");
        ramSb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar s, int p, boolean u) {
                int mb = ramSteps[Math.min(p, ramSteps.length - 1)];
                String label = mb >= 1024 ? "RAM: " + (mb/1024) + " GB" : "RAM: " + mb + " MB";
                ramVal.setText(label);
                updatePreview();
            }
            public void onStartTrackingTouch(SeekBar s) {}
            public void onStopTrackingTouch(SeekBar s) {}
        });
    }

    // ── Arch-driven dynamic spinner setup ─────────────────────────────
    private void setupArchListener() {
        if (archSpinner == null) return;
        archSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                triggerArchUpdate(pos);
            }
            public void onNothingSelected(AdapterView<?> p) {}
        });
    }

    private void triggerArchUpdate(int archPos) {
        ArchGroup grp = archGroup(archPos);

        // Machine
        populateSpinner(machSpinner, machArrayForGroup(grp));

        // CPU
        populateSpinner(cpuSpinner, cpuArrayForGroup(grp));

        // GPU
        populateSpinner(gpuSpinner, gpuArrayForGroup(grp));

        // Audio
        populateSpinner(audioSpinner, audioArrayForGroup(grp));

        // Accelerator
        populateSpinner(accelSpinner, accelArrayForGroup(grp));

        // LBT note only for LoongArch
        if (accelSpinner != null) {
            accelSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                    String sel = (String) p.getItemAtPosition(pos);
                    if (lbtNoteText != null)
                        lbtNoteText.setVisibility(
                            sel != null && sel.startsWith("KVM + LBT") ? View.VISIBLE : View.GONE);
                    updatePreview();
                }
                public void onNothingSelected(AdapterView<?> p) {}
            });
        }

        // Machine custom field
        if (machSpinner != null) {
            machSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                    String sel = (String) p.getItemAtPosition(pos);
                    if (machCustomEntry != null)
                        machCustomEntry.setVisibility(
                            "custom".equals(sel) ? View.VISIBLE : View.GONE);
                    updatePreview();
                }
                public void onNothingSelected(AdapterView<?> p) {}
            });
        }
    }

    private void populateSpinner(Spinner sp, int arrayResId) {
        if (sp == null) return;
        String[] items = getResources().getStringArray(arrayResId);
        ArrayAdapter<String> ad = new ArrayAdapter<>(this,
            android.R.layout.simple_spinner_item, items);
        ad.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        sp.setAdapter(ad);
    }

    private int machArrayForGroup(ArchGroup g) {
        switch (g) {
            case X86: case I386: return R.array.machine_x86;
            case AARCH64: case ARM: return R.array.machine_arm;
            case RISCV64: case RISCV32: return R.array.machine_riscv;
            case MIPS: return R.array.machine_mips;
            case PPC: return R.array.machine_ppc;
            case S390: return R.array.machine_s390;
            case LOONG: return R.array.machine_loong;
            default: return R.array.machine_generic;
        }
    }

    private int cpuArrayForGroup(ArchGroup g) {
        switch (g) {
            case X86: return R.array.cpu_x86_64;
            case I386: return R.array.cpu_i386;
            case AARCH64: return R.array.cpu_aarch64;
            case ARM: return R.array.cpu_arm;
            case RISCV64: return R.array.cpu_riscv64;
            case RISCV32: return R.array.cpu_riscv32;
            case MIPS: return R.array.cpu_mips;
            case PPC: return R.array.cpu_ppc;
            case S390: return R.array.cpu_s390;
            case LOONG: return R.array.cpu_loong;
            default: return R.array.cpu_generic;
        }
    }

    private int gpuArrayForGroup(ArchGroup g) {
        switch (g) {
            case X86: case I386: return R.array.gpu_x86;
            case AARCH64: case ARM: return R.array.gpu_arm;
            default: return R.array.gpu_minimal;
        }
    }

    private int audioArrayForGroup(ArchGroup g) {
        switch (g) {
            case X86: case I386: return R.array.audio_x86;
            case AARCH64: case ARM: return R.array.audio_arm;
            default: return R.array.audio_generic;
        }
    }

    private int accelArrayForGroup(ArchGroup g) {
        switch (g) {
            case X86: case I386: return R.array.accel_x86;
            case AARCH64: case ARM: return R.array.accel_arm;
            default: return R.array.accel_generic;
        }
    }

    // ── Preview ────────────────────────────────────────────────────────
    private void updatePreview() {
        try {
            VMConfig c = collectConfig();
            if (cmdPreview != null)
                cmdPreview.setText(CommandBuilder.formatCommand(c));
        } catch (Exception ignored) {}
    }

    // ── Collect form into VMConfig ─────────────────────────────────────
    private VMConfig collectConfig() {
        VMConfig c = new VMConfig();

        c.name        = nameEntry != null ? nameEntry.getText().toString().trim() : "";
        c.description = descEntry != null ? descEntry.getText().toString().trim() : "";

        // Arch
        int archPos = archSpinner != null ? archSpinner.getSelectedItemPosition() : 0;
        VMConfig.VMArch[] archs = VMConfig.VMArch.values();
        c.arch = archs[Math.min(archPos, archs.length - 1)];

        // Mode
        c.mode = modeSpinner != null && modeSpinner.getSelectedItemPosition() == 1
            ? VMConfig.VMMode.UserMode : VMConfig.VMMode.System;

        // Machine
        if (machSpinner != null) {
            String msel = (String) machSpinner.getSelectedItem();
            if (msel == null) msel = "q35";
            if (msel.startsWith("q35"))              c.machine = VMConfig.MachineType.q35;
            else if (msel.startsWith("pc"))          c.machine = VMConfig.MachineType.pc;
            else if (msel.startsWith("virt-acpi"))   c.machine = VMConfig.MachineType.virt_acpi;
            else if (msel.startsWith("virt"))        c.machine = VMConfig.MachineType.virt;
            else if (msel.startsWith("microvm"))     c.machine = VMConfig.MachineType.microvm;
            else if (msel.startsWith("x86-microvm")) c.machine = VMConfig.MachineType.x86_64_microvm;
            else if (msel.startsWith("nitro"))       c.machine = VMConfig.MachineType.nitro_enclave;
            else if (msel.startsWith("sbsa"))        c.machine = VMConfig.MachineType.sbsa_ref;
            else if (msel.startsWith("custom"))      c.machine = VMConfig.MachineType.custom;
            else                                      c.machine = VMConfig.MachineType.custom;
            if (c.machine == VMConfig.MachineType.custom && machCustomEntry != null)
                c.machineCustom = machCustomEntry.getText().toString().trim();
        }

        // CPU — strip display suffix e.g. " (recommended)" → use raw name
        if (cpuSpinner != null) {
            String raw = (String) cpuSpinner.getSelectedItem();
            if (raw == null) raw = "max";
            int paren = raw.indexOf(" (");
            c.cpuModel = paren >= 0 ? raw.substring(0, paren).trim() : raw.trim();
            // Normalise QEMU 10 display names
            if (c.cpuModel.equals("DiamondRapids")) c.cpuModel = "DiamondRapids";
            if (c.cpuModel.equals("gen17"))         c.cpuModel = "gen17";
        }
        c.cpuFlags = cpuFlagsEntry != null ? cpuFlagsEntry.getText().toString().trim() : "";

        // SMP
        c.sockets = socketsSb != null ? socketsSb.getProgress() + 1 : 1;
        c.cores   = coresSb   != null ? coresSb.getProgress()   + 1 : 2;
        c.threads = threadsSb != null ? threadsSb.getProgress() + 1 : 2;

        // RAM
        int ri = ramSb != null ? Math.min(ramSb.getProgress(), ramSteps.length - 1) : 4;
        c.ramMb = ramSteps[ri];

        // Storage
        c.diskSizeGb = diskSizeEntry != null && !diskSizeEntry.getText().toString().isEmpty()
            ? Integer.parseInt(diskSizeEntry.getText().toString()) : 20;
        c.isoPath  = isoEntry != null ? isoEntry.getText().toString().trim() : "";

        // Disk format
        if (diskFmtSpinner != null) {
            String[] fmtMap = {"qcow2","raw","vmdk","vdi"};
            int fi = diskFmtSpinner.getSelectedItemPosition();
            try { c.diskFormat = VMConfig.DiskFormat.valueOf(fmtMap[fi < 4 ? fi : 0]); } catch (Exception ignored) {}
        }

        // Boot
        c.uefi     = uefiCheck     != null && uefiCheck.isChecked();
        c.bootMenu = bootMenuCheck != null && bootMenuCheck.isChecked();
        String[] bos = {"cd","dc","c","d"};
        c.bootOrder = bos[bootSpinner != null ? Math.min(bootSpinner.getSelectedItemPosition(), 3) : 0];

        // Display
        if (displaySpinner != null) {
            String[] dispMap = {"SDL","GTK","SPICE","VNC","EGL","Headless"};
            int di = displaySpinner.getSelectedItemPosition();
            try { c.display = VMConfig.DisplayType.valueOf(dispMap[di < 6 ? di : 0]); } catch (Exception ignored) {}
        }

        // GPU — map display label → enum
        if (gpuSpinner != null) {
            String gsel = (String) gpuSpinner.getSelectedItem();
            if (gsel == null) gsel = "";
            if (gsel.contains("GL"))               c.gpu = VMConfig.GPUType.VirtIO_GPU_GL;
            else if (gsel.contains("Rutabaga"))    c.gpu = VMConfig.GPUType.VirtIO_GPU_Rutabaga;
            else if (gsel.contains("NativeCtx"))   c.gpu = VMConfig.GPUType.VirtIO_GPU_NativeCtx;
            else if (gsel.contains("VirtIO"))      c.gpu = VMConfig.GPUType.VirtIO_GPU;
            else if (gsel.contains("QXL"))         c.gpu = VMConfig.GPUType.QXL;
            else if (gsel.contains("VMware"))      c.gpu = VMConfig.GPUType.VMwareSVGA;
            else if (gsel.contains("Cirrus"))      c.gpu = VMConfig.GPUType.Cirrus;
            else if (gsel.contains("ramfb"))       c.gpu = VMConfig.GPUType.ramfb;
            else if (gsel.contains("None"))        c.gpu = VMConfig.GPUType.None;
            else                                   c.gpu = VMConfig.GPUType.VGA;
        }

        // Audio — map display label → enum
        if (audioSpinner != null) {
            String asel = (String) audioSpinner.getSelectedItem();
            if (asel == null) asel = "";
            if (asel.contains("Intel HDA"))        c.audio = VMConfig.AudioType.IntelHDA;
            else if (asel.contains("AC97"))        c.audio = VMConfig.AudioType.AC97;
            else if (asel.contains("SB16"))        c.audio = VMConfig.AudioType.SB16;
            else if (asel.contains("VirtIO"))      c.audio = VMConfig.AudioType.VirtIO_Sound;
            else                                   c.audio = VMConfig.AudioType.None;
        }

        // Network
        String[] netMap = {"User","TAP","Bridge","Socket"};
        int ni = netSpinner != null ? netSpinner.getSelectedItemPosition() : 0;
        try { c.net = VMConfig.NetworkMode.valueOf(netMap[ni < 4 ? ni : 0]); } catch (Exception ignored) {}

        // Accelerator
        if (accelSpinner != null) {
            String asel = (String) accelSpinner.getSelectedItem();
            if (asel == null) asel = "";
            if      (asel.startsWith("KVM + LBT")) c.accel = VMConfig.Accelerator.KVM_LBT;
            else if (asel.startsWith("KVM"))        c.accel = VMConfig.Accelerator.KVM;
            else if (asel.startsWith("WHPX"))       c.accel = VMConfig.Accelerator.WHPX;
            else if (asel.startsWith("HVF"))        c.accel = VMConfig.Accelerator.HVF;
            else if (asel.startsWith("Nitro"))      c.accel = VMConfig.Accelerator.Nitro;
            else if (asel.startsWith("MSHV"))       c.accel = VMConfig.Accelerator.MSHV;
            else                                    c.accel = VMConfig.Accelerator.TCG;
        }

        // Extra args
        c.extraArgs = extraArgsEntry != null ? extraArgsEntry.getText().toString().trim() : "";
        c.saveScriptToFolder = saveScriptCheck != null && saveScriptCheck.isChecked();

        // Binary: always auto — no custom binary needed when arch is fixed
        c.binaryMode   = VMConfig.BinaryMode.Auto;
        c.customBinary = "";

        return c;
    }

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

            VMConfigIO.saveConfig(c);
            setResult(RESULT_OK);
            Toast.makeText(this, "VM created: " + c.name, Toast.LENGTH_SHORT).show();
            finish();
        } catch (Exception e) {
            Toast.makeText(this, "Error: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }
}

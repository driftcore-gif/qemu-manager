package com.qemumanager.ui;

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
    private EditText  cpuEntry;
    private SeekBar   socketsSb, coresSb, threadsSb;
    private TextView  socketsVal, coresVal, threadsVal;
    private SeekBar   ramSb;
    private TextView  ramVal;
    private CheckBox  balloonCheck;
    private EditText  diskSizeEntry, isoEntry;
    private Spinner   diskFmtSpinner;
    private CheckBox  uefiCheck, bootMenuCheck;
    private Spinner   bootSpinner;
    private Spinner   displaySpinner, gpuSpinner, audioSpinner;
    private Spinner   netSpinner;
    private Spinner   binSpinner, accelSpinner;
    private EditText  customBinEntry, extraArgsEntry;
    private TextView  binHintText, lbtNoteText;
    private CheckBox  saveScriptCheck;
    private LinearLayout customBinRow;
    private TextView cmdPreview;

    private String vmBaseDir;
    private List<String> detectedBins = new ArrayList<>();
    private int[] ramSteps = {64,128,256,512,1024,2048,4096,8192,16384,32768};

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
            setupHardwareTab();
            setupPreviewListener();

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
        cpuEntry       = findViewById(R.id.et_cpu);
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
        gpuSpinner     = findViewById(R.id.sp_gpu);
        audioSpinner   = findViewById(R.id.sp_audio);
        netSpinner     = findViewById(R.id.sp_net);
        binSpinner     = findViewById(R.id.sp_binary);
        accelSpinner   = findViewById(R.id.sp_accel);
        customBinEntry = findViewById(R.id.et_custom_bin);
        extraArgsEntry = findViewById(R.id.et_extra_args);
        binHintText    = findViewById(R.id.tv_bin_hint);
        lbtNoteText    = findViewById(R.id.tv_lbt_note);
        saveScriptCheck= findViewById(R.id.cb_save_script);
        customBinRow   = findViewById(R.id.row_custom_bin);
        cmdPreview     = findViewById(R.id.tv_cmd_preview);

        setupSeekBar(socketsSb, socketsVal, 1, 8, 1, "Sockets: ");
        setupSeekBar(coresSb,   coresVal,   1, 64, 2, "Cores: ");
        setupSeekBar(threadsSb, threadsVal, 1, 8, 2, "Threads: ");
        setupRamSeekBar();

        if (machSpinner != null) {
            machSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                    if (machCustomEntry != null)
                        machCustomEntry.setVisibility(pos == 4 ? View.VISIBLE : View.GONE);
                }
                public void onNothingSelected(AdapterView<?> p) {}
            });
        }
    }

    private void setupSeekBar(SeekBar sb, TextView tv, int min, int max, int def, String label) {
        if (sb == null || tv == null) return;
        sb.setMax(max - min);
        sb.setProgress(def - min);
        tv.setText(label + def);
        sb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar s, int p, boolean u) { tv.setText(label + (p + min)); }
            public void onStartTrackingTouch(SeekBar s) {}
            public void onStopTrackingTouch(SeekBar s) {}
        });
    }

    private void setupRamSeekBar() {
        if (ramSb == null || ramVal == null) return;
        ramSb.setMax(ramSteps.length - 1);
        ramSb.setProgress(5);
        ramVal.setText("RAM: 2048 MB");
        ramSb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar s, int p, boolean u) {
                ramVal.setText("RAM: " + ramSteps[Math.min(p, ramSteps.length-1)] + " MB");
            }
            public void onStartTrackingTouch(SeekBar s) {}
            public void onStopTrackingTouch(SeekBar s) {}
        });
    }

    private void setupHardwareTab() {
        detectedBins = CommandBuilder.listInstalledBinaries(VMConfig.VMMode.System);
        if (detectedBins == null) detectedBins = new ArrayList<>();
        
        List<String> items = new ArrayList<>(detectedBins);
        if (detectedBins.isEmpty()) {
            // No QEMU found — add a default so the spinner isn't empty
            items.add("Auto (qemu-system-)");
        }
        items.add("Custom…");
        
        ArrayAdapter<String> binAdapter = new ArrayAdapter<>(this,
            android.R.layout.simple_spinner_item, items);
        binAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        if (binSpinner != null) binSpinner.setAdapter(binAdapter);

        if (binSpinner != null) {
            binSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                    boolean isCustom = pos == items.size() - 1;
                    if (customBinRow != null)
                        customBinRow.setVisibility(isCustom ? View.VISIBLE : View.GONE);
                    if (binHintText != null) binHintText.setText("");
                }
                public void onNothingSelected(AdapterView<?> p) {}
            });
        }

        if (customBinEntry != null) {
            customBinEntry.addTextChangedListener(new TextWatcher() {
                public void beforeTextChanged(CharSequence s, int st, int c, int a) {}
                public void afterTextChanged(Editable s) {}
                public void onTextChanged(CharSequence s, int st, int b, int c) {
                    String txt = s.toString();
                    if (txt.contains(" ") || txt.contains("/")) {
                        if (binHintText != null)
                            binHintText.setText("⚠ Binary name only — no spaces or slashes");
                        customBinEntry.setError("Invalid");
                    } else {
                        if (binHintText != null) binHintText.setText("");
                        customBinEntry.setError(null);
                    }
                }
            });
        }

        if (accelSpinner != null) {
            accelSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                    if (lbtNoteText != null)
                        lbtNoteText.setVisibility(pos == 2 ? View.VISIBLE : View.GONE);
                }
                public void onNothingSelected(AdapterView<?> p) {}
            });
        }
    }

    private void setupPreviewListener() {
        if (nameEntry != null) {
            nameEntry.addTextChangedListener(new TextWatcher() {
                public void beforeTextChanged(CharSequence s, int st, int c, int a) {}
                public void afterTextChanged(Editable s) { updatePreview(); }
                public void onTextChanged(CharSequence s, int st, int b, int c) {}
            });
        }
    }

    private void updatePreview() {
        try {
            VMConfig c = collectConfig();
            if (cmdPreview != null)
                cmdPreview.setText(CommandBuilder.formatCommand(c));
        } catch (Exception ignored) {}
    }

    private VMConfig collectConfig() {
        VMConfig c = new VMConfig();
        c.name        = nameEntry != null ? nameEntry.getText().toString().trim() : "";
        c.description = descEntry != null ? descEntry.getText().toString().trim() : "";
        if (archSpinner != null)
            c.arch = VMConfig.VMArch.values()[Math.min(archSpinner.getSelectedItemPosition(), VMConfig.VMArch.values().length-1)];
        if (modeSpinner != null)
            c.mode = modeSpinner.getSelectedItemPosition() == 1
                    ? VMConfig.VMMode.UserMode : VMConfig.VMMode.System;
        VMConfig.MachineType[] mts = {VMConfig.MachineType.q35, VMConfig.MachineType.pc,
            VMConfig.MachineType.virt, VMConfig.MachineType.microvm, VMConfig.MachineType.custom};
        if (machSpinner != null)
            c.machine = mts[Math.min(machSpinner.getSelectedItemPosition(), mts.length-1)];
        c.machineCustom = machCustomEntry != null ? machCustomEntry.getText().toString().trim() : "";
        c.cpuModel      = cpuEntry != null ? cpuEntry.getText().toString().trim() : "max";
        c.sockets       = socketsSb != null ? socketsSb.getProgress() + 1 : 1;
        c.cores         = coresSb != null ? coresSb.getProgress() + 1 : 2;
        c.threads       = threadsSb != null ? threadsSb.getProgress() + 1 : 2;
        c.ramMb         = ramSteps[Math.min(ramSb != null ? ramSb.getProgress() : 5, ramSteps.length-1)];
        c.ballooning    = balloonCheck != null && balloonCheck.isChecked();
        try { c.diskSizeGb = Integer.parseInt(diskSizeEntry.getText().toString()); } catch(Exception ignored){ c.diskSizeGb = 20; }
        if (diskFmtSpinner != null)
            c.diskFormat = VMConfig.DiskFormat.values()[Math.min(diskFmtSpinner.getSelectedItemPosition(), VMConfig.DiskFormat.values().length-1)];
        c.isoPath       = isoEntry != null ? isoEntry.getText().toString().trim() : "";
        c.uefi          = uefiCheck != null && uefiCheck.isChecked();
        c.bootMenu      = bootMenuCheck != null && bootMenuCheck.isChecked();
        String[] bos    = {"cd","dc","c","d"};
        if (bootSpinner != null)
            c.bootOrder = bos[Math.min(bootSpinner.getSelectedItemPosition(), bos.length-1)];
        if (displaySpinner != null)
            c.display = VMConfig.DisplayType.values()[Math.min(displaySpinner.getSelectedItemPosition(), VMConfig.DisplayType.values().length-1)];
        if (gpuSpinner != null)
            c.gpu = VMConfig.GPUType.values()[Math.min(gpuSpinner.getSelectedItemPosition(), VMConfig.GPUType.values().length-1)];
        if (audioSpinner != null)
            c.audio = VMConfig.AudioType.values()[Math.min(audioSpinner.getSelectedItemPosition(), VMConfig.AudioType.values().length-1)];
        if (netSpinner != null)
            c.net = VMConfig.NetworkMode.values()[Math.min(netSpinner.getSelectedItemPosition(), VMConfig.NetworkMode.values().length-1)];

        int sel = binSpinner != null ? binSpinner.getSelectedItemPosition() : 0;
        
        if (binSpinner != null && sel == binSpinner.getAdapter().getCount() - 1) {
            c.binaryMode   = VMConfig.BinaryMode.Custom;
            c.customBinary = customBinEntry != null ? customBinEntry.getText().toString().trim() : "";
        } else {
            c.binaryMode   = VMConfig.BinaryMode.Auto;
            c.customBinary = "";
        }

        VMConfig.Accelerator[] accels = VMConfig.Accelerator.values();
        if (accelSpinner != null)
            c.accel = accels[Math.min(accelSpinner.getSelectedItemPosition(), accels.length-1)];

        c.extraArgs          = extraArgsEntry != null ? extraArgsEntry.getText().toString().trim() : "";
        c.saveScriptToFolder = saveScriptCheck != null && saveScriptCheck.isChecked();

        // Sanitize VM name for directory — replace spaces with underscores
        String safeName = c.name.replaceAll("[^A-Za-z0-9_\\-]", "_");
        if (vmBaseDir != null) c.vmDir = vmBaseDir + "/" + safeName;
        String[] exts = {"qcow2","raw","vmdk","vdi"};
        c.diskPath = c.vmDir + "/disk." + exts[c.diskFormat.ordinal()];
        return c;
    }

    private void onCreateClicked() {
        try {
            VMConfig c = collectConfig();
            if (c.name.isEmpty()) {
                if (nameEntry != null) { nameEntry.setError("VM name is required"); nameEntry.requestFocus(); }
                return;
            }
            if (c.binaryMode == VMConfig.BinaryMode.Custom) {
                String err = CommandBuilder.validateCustomBinary(c.customBinary);
                if (!err.isEmpty()) {
                    if (customBinEntry != null) { customBinEntry.setError(err); customBinEntry.requestFocus(); }
                    return;
                }
            }

            // Create directories
            File vmDir = new File(c.vmDir);
            if (!vmDir.exists() && !vmDir.mkdirs()) {
                Toast.makeText(this, "Failed to create VM directory: " + c.vmDir, Toast.LENGTH_LONG).show();
                return;
            }
            new File(c.vmDir + "/snapshots").mkdirs();
            new File(c.vmDir + "/logs").mkdirs();

            // Save with error checking
            String error = VMConfigIO.saveWithError(c);
            if (error != null) {
                Toast.makeText(this, "Failed to save VM: " + error, Toast.LENGTH_LONG).show();
                return;
            }

            if (c.saveScriptToFolder) VMConfigIO.writeStartScript(c);
            Toast.makeText(this, "VM '" + c.name + "' created!", Toast.LENGTH_SHORT).show();
            setResult(RESULT_OK);
            finish();
        } catch (Exception e) {
            Toast.makeText(this, "Error creating VM: " + e.getMessage(), Toast.LENGTH_LONG).show();
            e.printStackTrace();
        }
    }
}

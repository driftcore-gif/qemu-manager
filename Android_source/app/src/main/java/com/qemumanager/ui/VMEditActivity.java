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

public class VMEditActivity extends AppCompatActivity {

    // General
    private EditText nameEntry, descEntry, machCustomEntry, cpuEntry;
    private Spinner archSpinner, modeSpinner, machSpinner;
    // CPU
    private SeekBar socketsSb, coresSb, threadsSb;
    private TextView socketsVal, coresVal, threadsVal;
    // Memory
    private SeekBar ramSb;
    private TextView ramVal;
    private CheckBox balloonCheck;
    // Storage
    private EditText diskSizeEntry, isoEntry;
    private Spinner diskFmtSpinner;
    // Boot
    private CheckBox uefiCheck, bootMenuCheck;
    private Spinner bootSpinner;
    // Display
    private Spinner displaySpinner, gpuSpinner, audioSpinner;
    // Network
    private Spinner netSpinner;
    // Hardware
    private Spinner binSpinner, accelSpinner;
    private EditText customBinEntry, extraArgsEntry;
    private TextView binHintText, lbtNoteText;
    private CheckBox saveScriptCheck;
    private LinearLayout customBinRow;
    // Preview
    private TextView cmdPreview;

    private VMConfig vm;
    private List<String> detectedBins = new ArrayList<>();
    private List<String> spinnerItems = new ArrayList<>();
    private int[] ramSteps = {64,128,256,512,1024,2048,4096,8192,16384,32768};

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            setContentView(R.layout.activity_vm_edit);
            String vmDir = getIntent().getStringExtra("vm_dir");
            if (vmDir == null) { finish(); return; }
            vm = VMConfigIO.load(vmDir);
            if (vm == null) { Toast.makeText(this, "Failed to load VM", Toast.LENGTH_SHORT).show(); finish(); return; }

            if (getSupportActionBar() != null)
                getSupportActionBar().setTitle("Edit - " + vm.name);

            bindViews();
            populateFromVM();
            setupListeners();

            Button save = findViewById(R.id.btn_save);
            Button cancel = findViewById(R.id.btn_cancel_edit);
            if (save != null) save.setOnClickListener(v -> saveChanges());
            if (cancel != null) cancel.setOnClickListener(v -> finish());
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
        cpuEntry       = findViewById(R.id.et_cpu);
        archSpinner    = findViewById(R.id.sp_arch);
        modeSpinner    = findViewById(R.id.sp_mode);
        machSpinner    = findViewById(R.id.sp_machine);
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
    }

    private void populateFromVM() {
        if (nameEntry != null) nameEntry.setText(vm.name);
        if (descEntry != null) descEntry.setText(vm.description);
        if (archSpinner != null) archSpinner.setSelection(vm.arch.ordinal());
        if (modeSpinner != null) modeSpinner.setSelection(vm.mode == VMConfig.VMMode.UserMode ? 1 : 0);

        VMConfig.MachineType[] mts = {VMConfig.MachineType.q35, VMConfig.MachineType.pc,
            VMConfig.MachineType.virt, VMConfig.MachineType.microvm, VMConfig.MachineType.custom};
        for (int i = 0; i < mts.length; i++) {
            if (mts[i] == vm.machine) { if (machSpinner != null) machSpinner.setSelection(i); break; }
        }
        if (machCustomEntry != null) {
            machCustomEntry.setText(vm.machineCustom);
            machCustomEntry.setVisibility(vm.machine == VMConfig.MachineType.custom ? View.VISIBLE : View.GONE);
        }
        if (cpuEntry != null) cpuEntry.setText(vm.cpuModel);

        // SeekBars
        if (socketsSb != null) { socketsSb.setMax(7); socketsSb.setProgress(vm.sockets - 1); socketsVal.setText("Sockets: " + vm.sockets); }
        if (coresSb != null) { coresSb.setMax(63); coresSb.setProgress(vm.cores - 1); coresVal.setText("Cores: " + vm.cores); }
        if (threadsSb != null) { threadsSb.setMax(7); threadsSb.setProgress(vm.threads - 1); threadsVal.setText("Threads: " + vm.threads); }

        // RAM - find closest step
        if (ramSb != null && ramVal != null) {
            ramSb.setMax(ramSteps.length - 1);
            int closest = 5; // default 2048
            for (int i = 0; i < ramSteps.length; i++) {
                if (ramSteps[i] == vm.ramMb) { closest = i; break; }
                if (ramSteps[i] < vm.ramMb) closest = i;
            }
            ramSb.setProgress(closest);
            ramVal.setText("RAM: " + ramSteps[closest] + " MB");
        }
        if (balloonCheck != null) balloonCheck.setChecked(vm.ballooning);

        if (diskSizeEntry != null) diskSizeEntry.setText(String.valueOf(vm.diskSizeGb));
        if (diskFmtSpinner != null) diskFmtSpinner.setSelection(vm.diskFormat.ordinal());
        if (isoEntry != null) isoEntry.setText(vm.isoPath);

        if (uefiCheck != null) uefiCheck.setChecked(vm.uefi);
        if (bootMenuCheck != null) bootMenuCheck.setChecked(vm.bootMenu);
        String[] bos = {"cd","dc","c","d"};
        for (int i = 0; i < bos.length; i++) {
            if (bos[i].equals(vm.bootOrder)) { if (bootSpinner != null) bootSpinner.setSelection(i); break; }
        }

        if (displaySpinner != null) displaySpinner.setSelection(vm.display.ordinal());
        if (gpuSpinner != null) gpuSpinner.setSelection(vm.gpu.ordinal());
        if (audioSpinner != null) audioSpinner.setSelection(vm.audio.ordinal());
        if (netSpinner != null) netSpinner.setSelection(vm.net.ordinal());

        // Hardware - binary
        detectedBins = CommandBuilder.listInstalledBinaries(vm.mode);
        if (detectedBins == null) detectedBins = new ArrayList<>();
        spinnerItems = new ArrayList<>(detectedBins);
        if (detectedBins.isEmpty()) spinnerItems.add("(No QEMU found)");
        spinnerItems.add("Custom...");

        ArrayAdapter<String> a = new ArrayAdapter<>(this,
            android.R.layout.simple_spinner_item, spinnerItems);
        a.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        if (binSpinner != null) binSpinner.setAdapter(a);

        if (vm.binaryMode == VMConfig.BinaryMode.Custom) {
            if (binSpinner != null) binSpinner.setSelection(spinnerItems.size() - 1);
            if (customBinRow != null) customBinRow.setVisibility(View.VISIBLE);
            if (customBinEntry != null) customBinEntry.setText(vm.customBinary);
        } else {
            int idx = detectedBins.indexOf(CommandBuilder.archToQemuBin(vm.arch, vm.mode));
            if (idx >= 0 && binSpinner != null) binSpinner.setSelection(idx);
        }

        if (accelSpinner != null) accelSpinner.setSelection(vm.accel.ordinal());
        if (lbtNoteText != null) lbtNoteText.setVisibility(vm.accel == VMConfig.Accelerator.KVM_LBT ? View.VISIBLE : View.GONE);
        if (extraArgsEntry != null) extraArgsEntry.setText(vm.extraArgs);
        if (saveScriptCheck != null) saveScriptCheck.setChecked(vm.saveScriptToFolder);
    }

    private void setupListeners() {
        // Machine custom visibility
        if (machSpinner != null) {
            machSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                    if (machCustomEntry != null)
                        machCustomEntry.setVisibility(pos == 4 ? View.VISIBLE : View.GONE);
                }
                public void onNothingSelected(AdapterView<?> p) {}
            });
        }

        // SeekBars
        setupSeekBar(socketsSb, socketsVal, 1, 8, vm.sockets, "Sockets: ");
        setupSeekBar(coresSb, coresVal, 1, 64, vm.cores, "Cores: ");
        setupSeekBar(threadsSb, threadsVal, 1, 8, vm.threads, "Threads: ");
        if (ramSb != null && ramVal != null) {
            ramSb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                public void onProgressChanged(SeekBar s, int p, boolean u) {
                    ramVal.setText("RAM: " + ramSteps[Math.min(p, ramSteps.length-1)] + " MB");
                }
                public void onStartTrackingTouch(SeekBar s) {}
                public void onStopTrackingTouch(SeekBar s) {}
            });
        }

        // Binary spinner
        if (binSpinner != null) {
            binSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                    boolean isCustom = pos == spinnerItems.size() - 1;
                    if (customBinRow != null) customBinRow.setVisibility(isCustom ? View.VISIBLE : View.GONE);
                    if (binHintText != null) binHintText.setText("");
                }
                public void onNothingSelected(AdapterView<?> p) {}
            });
        }

        // Custom binary validation
        if (customBinEntry != null) {
            customBinEntry.addTextChangedListener(new TextWatcher() {
                public void beforeTextChanged(CharSequence s, int st, int c, int a) {}
                public void afterTextChanged(Editable s) {}
                public void onTextChanged(CharSequence s, int st, int b, int c) {
                    String txt = s.toString();
                    if (txt.contains(" ") || txt.contains("/")) {
                        if (binHintText != null) binHintText.setText("Binary name only - no spaces or slashes");
                        customBinEntry.setError("Invalid");
                    } else {
                        if (binHintText != null) binHintText.setText("");
                        customBinEntry.setError(null);
                    }
                }
            });
        }

        // Accelerator LBT note
        if (accelSpinner != null) {
            accelSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                    if (lbtNoteText != null) lbtNoteText.setVisibility(pos == 2 ? View.VISIBLE : View.GONE);
                }
                public void onNothingSelected(AdapterView<?> p) {}
            });
        }

        // Name change updates preview
        if (nameEntry != null) {
            nameEntry.addTextChangedListener(new TextWatcher() {
                public void beforeTextChanged(CharSequence s, int st, int c, int a) {}
                public void afterTextChanged(Editable s) { updatePreview(); }
                public void onTextChanged(CharSequence s, int st, int b, int c) {}
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

    private void updatePreview() {
        try {
            VMConfig c = collectConfig();
            if (cmdPreview != null) cmdPreview.setText(CommandBuilder.formatCommand(c));
        } catch (Exception ignored) {}
    }

    private VMConfig collectConfig() {
        VMConfig c = vm; // Start from existing
        if (nameEntry != null) c.name = nameEntry.getText().toString().trim();
        if (descEntry != null) c.description = descEntry.getText().toString().trim();
        if (archSpinner != null)
            c.arch = VMConfig.VMArch.values()[Math.min(archSpinner.getSelectedItemPosition(), VMConfig.VMArch.values().length-1)];
        if (modeSpinner != null)
            c.mode = modeSpinner.getSelectedItemPosition() == 1 ? VMConfig.VMMode.UserMode : VMConfig.VMMode.System;
        VMConfig.MachineType[] mts = {VMConfig.MachineType.q35, VMConfig.MachineType.pc,
            VMConfig.MachineType.virt, VMConfig.MachineType.microvm, VMConfig.MachineType.custom};
        if (machSpinner != null)
            c.machine = mts[Math.min(machSpinner.getSelectedItemPosition(), mts.length-1)];
        if (machCustomEntry != null) c.machineCustom = machCustomEntry.getText().toString().trim();
        if (cpuEntry != null) c.cpuModel = cpuEntry.getText().toString().trim();
        c.sockets = socketsSb != null ? socketsSb.getProgress() + 1 : vm.sockets;
        c.cores = coresSb != null ? coresSb.getProgress() + 1 : vm.cores;
        c.threads = threadsSb != null ? threadsSb.getProgress() + 1 : vm.threads;
        c.ramMb = ramSteps[Math.min(ramSb != null ? ramSb.getProgress() : 5, ramSteps.length-1)];
        c.ballooning = balloonCheck != null && balloonCheck.isChecked();
        try { c.diskSizeGb = Integer.parseInt(diskSizeEntry.getText().toString()); } catch(Exception ignored) {}
        if (diskFmtSpinner != null)
            c.diskFormat = VMConfig.DiskFormat.values()[Math.min(diskFmtSpinner.getSelectedItemPosition(), VMConfig.DiskFormat.values().length-1)];
        if (isoEntry != null) c.isoPath = isoEntry.getText().toString().trim();
        c.uefi = uefiCheck != null && uefiCheck.isChecked();
        c.bootMenu = bootMenuCheck != null && bootMenuCheck.isChecked();
        String[] bos = {"cd","dc","c","d"};
        if (bootSpinner != null) c.bootOrder = bos[Math.min(bootSpinner.getSelectedItemPosition(), bos.length-1)];
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
            c.binaryMode = VMConfig.BinaryMode.Custom;
            c.customBinary = customBinEntry != null ? customBinEntry.getText().toString().trim() : "";
        } else {
            c.binaryMode = VMConfig.BinaryMode.Auto;
            c.customBinary = "";
        }
        if (accelSpinner != null)
            c.accel = VMConfig.Accelerator.values()[Math.min(accelSpinner.getSelectedItemPosition(), VMConfig.Accelerator.values().length-1)];
        c.extraArgs = extraArgsEntry != null ? extraArgsEntry.getText().toString().trim() : "";
        c.saveScriptToFolder = saveScriptCheck != null && saveScriptCheck.isChecked();

        // Preserve vmDir and diskPath
        c.vmDir = vm.vmDir;
        String[] exts = {"qcow2","raw","vmdk","vdi"};
        c.diskPath = c.vmDir + "/disk." + exts[c.diskFormat.ordinal()];

        return c;
    }

    private void saveChanges() {
        try {
            VMConfig c = collectConfig();
            if (c.name.isEmpty()) {
                if (nameEntry != null) { nameEntry.setError("VM name required"); nameEntry.requestFocus(); }
                return;
            }
            if (c.binaryMode == VMConfig.BinaryMode.Custom) {
                String err = CommandBuilder.validateCustomBinary(c.customBinary);
                if (!err.isEmpty()) {
                    if (customBinEntry != null) { customBinEntry.setError(err); customBinEntry.requestFocus(); }
                    return;
                }
            }
            String error = VMConfigIO.saveWithError(c);
            if (error != null) { Toast.makeText(this, "Save failed: " + error, Toast.LENGTH_LONG).show(); return; }
            if (c.saveScriptToFolder) VMConfigIO.writeStartScript(c);
            Toast.makeText(this, "Settings saved!", Toast.LENGTH_SHORT).show();
            setResult(RESULT_OK);
            finish();
        } catch (Exception e) {
            Toast.makeText(this, "Save error: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }
}

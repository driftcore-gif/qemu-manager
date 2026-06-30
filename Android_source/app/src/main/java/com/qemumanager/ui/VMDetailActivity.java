package com.qemumanager.ui;

import android.app.AlertDialog;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import com.qemumanager.R;
import com.qemumanager.model.VMConfig;
import com.qemumanager.util.CommandBuilder;
import com.qemumanager.util.VMConfigIO;
import java.io.File;

public class VMDetailActivity extends AppCompatActivity {

    private VMConfig vm;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            setContentView(R.layout.activity_vm_detail);

            String vmDir = getIntent().getStringExtra("vm_dir");
            if (vmDir == null) { finish(); return; }

            vm = VMConfigIO.load(vmDir);
            if (vm == null) {
                Toast.makeText(this, "Failed to load VM config", Toast.LENGTH_SHORT).show();
                finish();
                return;
            }

            if (getSupportActionBar() != null)
                getSupportActionBar().setTitle(vm.name);

            populateUI();

        } catch (Exception e) {
            Toast.makeText(this, "Error: " + e.getMessage(), Toast.LENGTH_LONG).show();
            finish();
        }
    }

    private void populateUI() {
        TextView tvName    = findViewById(R.id.tv_detail_name);
        TextView tvArch    = findViewById(R.id.tv_detail_arch);
        TextView tvCommand = findViewById(R.id.tv_detail_command);
        Button   btnCopy   = findViewById(R.id.btn_copy_cmd);
        Button   btnEdit   = findViewById(R.id.btn_edit);
        Button   btnDelete = findViewById(R.id.btn_delete);

        if (tvName != null)
            tvName.setText(vm.name);

        if (tvArch != null)
            tvArch.setText(vm.arch.name()
                + " · " + vm.mode.name()
                + " · " + vm.ramMb + " MB"
                + " · " + vm.accel.name());

        String cmd = CommandBuilder.formatCommand(vm);
        if (tvCommand != null)
            tvCommand.setText(cmd);

        if (btnCopy != null) {
            btnCopy.setOnClickListener(v -> {
                ClipboardManager cb = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                if (cb != null) {
                    // Copy the single-line version (no line-continuation backslashes)
                    cb.setPrimaryClip(ClipData.newPlainText("QEMU Command",
                            CommandBuilder.buildCommand(vm)));
                }
                Toast.makeText(this, "Command copied!", Toast.LENGTH_SHORT).show();
            });
        }

        if (btnEdit != null) {
            btnEdit.setOnClickListener(v -> {
                Intent i = new Intent(this, VMEditActivity.class);
                i.putExtra("vm_dir", vm.vmDir);
                startActivityForResult(i, 1);
            });
        }

        if (btnDelete != null) {
            btnDelete.setOnClickListener(v ->
                new AlertDialog.Builder(this)
                    .setTitle("Delete VM")
                    .setMessage("Delete \"" + vm.name + "\"? This cannot be undone.")
                    .setPositiveButton("Delete", (d, w) -> {
                        deleteDir(new File(vm.vmDir));
                        setResult(RESULT_OK);
                        finish();
                    })
                    .setNegativeButton("Cancel", null)
                    .show()
            );
        }
    }

    @Override
    protected void onActivityResult(int req, int res, Intent data) {
        super.onActivityResult(req, res, data);
        if (res == RESULT_OK) {
            // Reload config after edit and refresh the command box
            vm = VMConfigIO.load(vm.vmDir);
            if (vm != null) populateUI();
        }
    }

    private void deleteDir(File dir) {
        if (dir.isDirectory()) {
            File[] files = dir.listFiles();
            if (files != null) for (File f : files) deleteDir(f);
        }
        dir.delete();
    }
}

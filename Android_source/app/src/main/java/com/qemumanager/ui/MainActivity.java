package com.qemumanager.ui;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.qemumanager.R;
import com.qemumanager.adapter.VMAdapter;
import com.qemumanager.model.VMConfig;
import com.qemumanager.util.VMConfigIO;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    private RecyclerView recyclerView;
    private VMAdapter    adapter;
    private TextView     emptyText;
    private String       vmBaseDir;
    private static final int REQ_STORAGE = 100;
    private static final int REQ_NEW_VM = 1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            setContentView(R.layout.activity_main);
            if (findViewById(R.id.toolbar) != null) {
                setSupportActionBar(findViewById(R.id.toolbar));
            }

            // Use app-specific external storage — no permission needed on API 19+
            File extDir = getExternalFilesDir(null);
            if (extDir == null) extDir = getFilesDir();
            vmBaseDir = extDir.getAbsolutePath() + "/VMs";
            new File(vmBaseDir).mkdirs();

            recyclerView = findViewById(R.id.vm_list);
            emptyText    = findViewById(R.id.empty_text);
            if (recyclerView != null) {
                recyclerView.setLayoutManager(new LinearLayoutManager(this));
            }

            FloatingActionButton fab = findViewById(R.id.fab_new_vm);
            if (fab != null) {
                fab.setOnClickListener(v -> {
                    try {
                        Intent i = new Intent(this, VMWizardActivity.class);
                        i.putExtra("vm_base_dir", vmBaseDir);
                        startActivityForResult(i, REQ_NEW_VM);
                    } catch (Exception e) {
                        Toast.makeText(this, "Error: " + e.getMessage(), Toast.LENGTH_LONG).show();
                    }
                });
            }

            // Request storage permissions for older Android / MANAGE_EXTERNAL_STORAGE
            requestStoragePermission();
        } catch (Exception e) {
            Toast.makeText(this, "Fatal: " + e.getMessage(), Toast.LENGTH_LONG).show();
            e.printStackTrace();
        }
    }

    private void requestStoragePermission() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE,
                             Manifest.permission.READ_EXTERNAL_STORAGE}, REQ_STORAGE);
        }
    }

    @Override
    public void onRequestPermissionsResult(int req, @NonNull String[] p, @NonNull int[] r) {
        super.onRequestPermissionsResult(req, p, r);
        // Continue regardless — app-specific storage works without permissions
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        // Always reload VMs when returning from wizard or edit
        if (requestCode == REQ_NEW_VM) {
            loadVMs();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        loadVMs();
    }

    private void loadVMs() {
        try {
            List<VMConfig> vms = VMConfigIO.loadAll(vmBaseDir);
            if (vms == null) vms = new ArrayList<>();
            
            if (vms.isEmpty()) {
                if (emptyText != null) emptyText.setVisibility(View.VISIBLE);
                if (recyclerView != null) recyclerView.setVisibility(View.GONE);
            } else {
                if (emptyText != null) emptyText.setVisibility(View.GONE);
                if (recyclerView != null) {
                    recyclerView.setVisibility(View.VISIBLE);
                    adapter = new VMAdapter(vms, vm -> {
                        try {
                            Intent i = new Intent(this, VMDetailActivity.class);
                            i.putExtra("vm_dir", vm.vmDir);
                            startActivity(i);
                        } catch (Exception e) {
                            Toast.makeText(this, "Error: " + e.getMessage(), Toast.LENGTH_SHORT).show();
                        }
                    });
                    recyclerView.setAdapter(adapter);
                }
            }
        } catch (Exception e) {
            Toast.makeText(this, "Load error: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        if (item.getItemId() == R.id.action_settings) {
            startActivity(new Intent(this, SettingsActivity.class));
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}

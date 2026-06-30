package com.qemumanager.ui;

import android.os.Bundle;
import android.widget.*;
import androidx.appcompat.app.AppCompatActivity;
import com.qemumanager.R;
import com.qemumanager.util.VMLauncher;

public class QEMUMonitorActivity extends AppCompatActivity {

    private TextView  outputView;
    private EditText  cmdInput;
    private ScrollView scrollView;
    private String    vmDir;
    private StringBuilder log = new StringBuilder();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(Bundle.EMPTY);
        try {
            setContentView(R.layout.activity_monitor);
            vmDir = getIntent().getStringExtra("vm_dir");
            String vmName = getIntent().getStringExtra("vm_name");
            if (vmName == null) vmName = "VM";
            if (getSupportActionBar() != null)
                getSupportActionBar().setTitle("Monitor — " + vmName);

            outputView = findViewById(R.id.tv_monitor_output);
            cmdInput   = findViewById(R.id.et_monitor_cmd);
            scrollView = findViewById(R.id.scroll_monitor);

            appendLog("QEMU Monitor ready.\nType commands below or use quick buttons.\n\n");

            Button btnSend = findViewById(R.id.btn_send_cmd);
            if (btnSend != null) btnSend.setOnClickListener(v -> {
                String cmd = cmdInput != null ? cmdInput.getText().toString().trim() : "";
                if (cmd.isEmpty()) return;
                cmdInput.setText("");
                appendLog("> " + cmd + "\n");
                new Thread(() -> {
                    String result = VMLauncher.sendMonitorCommand(vmDir, cmd);
                    runOnUiThread(() -> appendLog(result));
                }).start();
            });

            int[] quickIds = {R.id.btn_info, R.id.btn_pause, R.id.btn_resume, R.id.btn_reset, R.id.btn_quit};
            String[] quickCmds = {"info status", "stop", "cont", "system_reset", "quit"};
            for (int i = 0; i < quickIds.length; i++) {
                Button b = findViewById(quickIds[i]);
                final String cmd = quickCmds[i];
                if (b != null) b.setOnClickListener(v -> sendQuick(cmd));
            }
        } catch (Exception e) {
            Toast.makeText(this, "Error: " + e.getMessage(), Toast.LENGTH_LONG).show();
            finish();
        }
    }

    private void sendQuick(String cmd) {
        appendLog("> " + cmd + "\n");
        new Thread(() -> {
            String result = VMLauncher.sendMonitorCommand(vmDir, cmd);
            runOnUiThread(() -> appendLog(result));
        }).start();
    }

    private void appendLog(String text) {
        log.append(text);
        if (outputView != null) outputView.setText(log.toString());
        if (scrollView != null)
            scrollView.post(() -> scrollView.fullScroll(android.view.View.FOCUS_DOWN));
    }
}

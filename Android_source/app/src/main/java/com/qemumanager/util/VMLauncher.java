package com.qemumanager.util;

import com.qemumanager.model.VMConfig;
import java.io.*;
import java.util.ArrayList;
import java.util.List;

public class VMLauncher {

    public interface LaunchCallback {
        void onSuccess(Process process, int pid);
        void onError(String message);
    }

    public static void launch(VMConfig vm, LaunchCallback cb) {
        if (vm.binaryMode == VMConfig.BinaryMode.Custom) {
            String err = CommandBuilder.validateCustomBinary(vm.customBinary);
            if (!err.isEmpty()) { cb.onError(err); return; }
        }

        if (vm.saveScriptToFolder) {
            try { VMConfigIO.writeStartScript(vm); } catch (Exception ignored) {}
        }

        new Thread(() -> {
            try {
                String bin = CommandBuilder.resolveBinary(vm);
                List<String> args = CommandBuilder.buildArgs(vm);
                List<String> cmd = new ArrayList<>();
                cmd.add(bin);
                cmd.addAll(args);

                ProcessBuilder pb = new ProcessBuilder(cmd);
                pb.directory(new File(vm.vmDir));
                pb.redirectErrorStream(true);
                Process proc = pb.start();
                cb.onSuccess(proc, 0);
            } catch (Exception e) {
                cb.onError("Launch failed: " + e.getMessage());
            }
        }).start();
    }

    public static String sendMonitorCommand(String vmDir, String cmd) {
        if (vmDir == null || vmDir.isEmpty()) return "Error: no VM directory";
        try {
            String sock = vmDir + "/monitor.sock";
            ProcessBuilder pb = new ProcessBuilder(
                "sh","-c","echo '" + cmd + "' | nc -U " + sock);
            pb.redirectErrorStream(true);
            Process p = pb.start();
            BufferedReader br = new BufferedReader(new InputStreamReader(p.getInputStream()));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = br.readLine()) != null) sb.append(line).append("\n");
            p.waitFor();
            return sb.toString();
        } catch (Exception e) { return "Error: " + e.getMessage(); }
    }
}

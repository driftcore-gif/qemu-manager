package com.qemumanager.util;

import android.os.AsyncTask;
import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

/**
 * Handles qemu-img disk creation, inspection, and selection.
 */
public class DiskManager {

    public enum DiskCreateMode {
        CREATE_NEW,       // Create a fresh qcow2/raw/vmdk/vdi with qemu-img
        USE_EXISTING,     // Point to an existing image file on device
        NO_DISK           // Boot from ISO only (live CD mode)
    }

    public interface DiskCreateCallback {
        void onSuccess(String diskPath);
        void onError(String message);
        void onProgress(String message);
    }

    // ── Resolve qemu-img path ──────────────────────────────────────────
    public static String resolveQemuImg() {
        String[] paths = {
            "/usr/bin/qemu-img",
            "/data/data/com.termux/files/usr/bin/qemu-img",
            "/system/bin/qemu-img",
            "/vendor/bin/qemu-img"
        };
        for (String p : paths) {
            if (new File(p).exists()) return p;
        }
        return "qemu-img"; // fallback — rely on PATH
    }

    public static boolean isQemuImgAvailable() {
        String path = resolveQemuImg();
        if (new File(path).exists()) return true;
        try {
            Process p = Runtime.getRuntime().exec(new String[]{"which", "qemu-img"});
            p.waitFor();
            return p.exitValue() == 0;
        } catch (Exception e) {
            return false;
        }
    }

    /**
     * Create a virtual disk image asynchronously.
     *
     * @param destPath  full path to create (e.g. /data/…/VMs/myvm/disk.qcow2)
     * @param sizeGb    disk size in GB
     * @param format    "qcow2", "raw", "vmdk", or "vdi"
     * @param preallocate  if true, use preallocation=full (faster IO, raw only)
     * @param cb        result callback (called on UI thread)
     */
    public static void createDiskAsync(
            String destPath, long sizeGb, String format,
            boolean preallocate, DiskCreateCallback cb) {

        new AsyncTask<Void, String, String>() {
            @Override protected void onPreExecute() {
                cb.onProgress("Creating " + sizeGb + " GB " + format.toUpperCase() + " disk…");
            }

            @Override protected String doInBackground(Void... v) {
                try {
                    // Make parent dirs
                    File dest = new File(destPath);
                    if (!dest.getParentFile().exists())
                        dest.getParentFile().mkdirs();

                    String qemuImg = resolveQemuImg();
                    List<String> cmd = new ArrayList<>();
                    cmd.add(qemuImg);
                    cmd.add("create");
                    cmd.add("-f"); cmd.add(format);

                    // Options
                    if (format.equals("qcow2")) {
                        // cluster_size=2M for large disks → better performance
                        if (sizeGb >= 64)
                            cmd.add("-o"); cmd.add("cluster_size=2M,compression_type=zstd");
                        // else default options
                    } else if (format.equals("raw") && preallocate) {
                        cmd.add("-o"); cmd.add("preallocation=full");
                    }

                    cmd.add(destPath);
                    cmd.add(sizeGb + "G");

                    publishProgress("Running: " + String.join(" ", cmd));

                    ProcessBuilder pb = new ProcessBuilder(cmd);
                    pb.redirectErrorStream(true);
                    Process proc = pb.start();

                    BufferedReader br = new BufferedReader(
                        new InputStreamReader(proc.getInputStream()));
                    StringBuilder out = new StringBuilder();
                    String line;
                    while ((line = br.readLine()) != null) {
                        out.append(line).append("\n");
                        publishProgress(line);
                    }
                    proc.waitFor();

                    if (proc.exitValue() != 0)
                        return "qemu-img failed:\n" + out;

                    if (!new File(destPath).exists())
                        return "Disk file not created at " + destPath;

                    return null; // success
                } catch (Exception e) {
                    return e.getMessage();
                }
            }

            @Override protected void onProgressUpdate(String... v) {
                cb.onProgress(v[0]);
            }

            @Override protected void onPostExecute(String err) {
                if (err == null) cb.onSuccess(destPath);
                else            cb.onError(err);
            }
        }.execute();
    }

    // ── Disk info (from qemu-img info) ───────────────────────────────
    public static class DiskInfo {
        public String path;
        public String format;
        public long   virtualSizeBytes;
        public long   actualSizeBytes;
        public String backingFile;
        public boolean valid;
        public String error;
    }

    public static DiskInfo inspectDisk(String path) {
        DiskInfo info = new DiskInfo();
        info.path = path;
        try {
            ProcessBuilder pb = new ProcessBuilder(
                resolveQemuImg(), "info", "--output=json", path);
            pb.redirectErrorStream(true);
            Process proc = pb.start();
            BufferedReader br = new BufferedReader(
                new InputStreamReader(proc.getInputStream()));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = br.readLine()) != null) sb.append(line);
            proc.waitFor();
            if (proc.exitValue() != 0) {
                info.valid = false;
                info.error = sb.toString();
                return info;
            }
            // Simple JSON parsing (no Gson needed)
            String json = sb.toString();
            info.format          = extractJsonStr(json, "format");
            info.virtualSizeBytes = extractJsonLong(json, "virtual-size");
            info.actualSizeBytes  = extractJsonLong(json, "actual-size");
            info.backingFile      = extractJsonStr(json, "backing-filename");
            info.valid = true;
        } catch (Exception e) {
            info.valid = false;
            info.error = e.getMessage();
        }
        return info;
    }

    private static String extractJsonStr(String json, String key) {
        try {
            int idx = json.indexOf("\"" + key + "\"");
            if (idx < 0) return "";
            int colon = json.indexOf(":", idx) + 1;
            while (colon < json.length() && json.charAt(colon) == ' ') colon++;
            if (json.charAt(colon) == '"') {
                int end = json.indexOf("\"", colon + 1);
                return json.substring(colon + 1, end);
            }
        } catch (Exception ignored) {}
        return "";
    }

    private static long extractJsonLong(String json, String key) {
        try {
            int idx = json.indexOf("\"" + key + "\"");
            if (idx < 0) return 0;
            int colon = json.indexOf(":", idx) + 1;
            while (colon < json.length() && json.charAt(colon) == ' ') colon++;
            int end = colon;
            while (end < json.length() && Character.isDigit(json.charAt(end))) end++;
            return Long.parseLong(json.substring(colon, end));
        } catch (Exception ignored) {}
        return 0;
    }

    // ── Scan for existing disk images in a directory ──────────────────
    public static List<String> scanForDiskImages(String dir) {
        List<String> found = new ArrayList<>();
        try {
            File d = new File(dir);
            if (!d.exists()) return found;
            for (File f : d.listFiles()) {
                String n = f.getName().toLowerCase();
                if (n.endsWith(".qcow2") || n.endsWith(".raw") ||
                    n.endsWith(".img")   || n.endsWith(".vmdk") ||
                    n.endsWith(".vdi")   || n.endsWith(".iso")) {
                    found.add(f.getAbsolutePath());
                }
            }
        } catch (Exception ignored) {}
        return found;
    }

    public static String humanSize(long bytes) {
        if (bytes <= 0) return "0 B";
        String[] u = {"B","KB","MB","GB","TB"};
        int i = 0;
        double v = bytes;
        while (v >= 1024 && i < 4) { v /= 1024; i++; }
        return String.format("%.1f %s", v, u[i]);
    }
}

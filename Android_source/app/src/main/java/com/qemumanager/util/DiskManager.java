package com.qemumanager.util;

import android.content.Context;
import android.net.Uri;
import android.os.AsyncTask;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * Handles qemu-img disk creation, inspection, selection, and conversion.
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

    public interface DiskConvertCallback {
        void onSuccess(String destPath);
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

    public static String resolveUri(Context context, Uri uri) {
        if (uri == null) return null;
        try {
            String scheme = uri.getScheme();
            if ("file".equals(scheme)) return uri.getPath();
            if ("content".equals(scheme)) {
                String path = uri.getPath();
                if (path != null && path.startsWith("/storage")) return path;
                List<String> segs = uri.getPathSegments();
                if (segs != null && segs.size() >= 2) {
                    String last = segs.get(segs.size() - 1);
                    if (last.contains(":")) {
                        String[] parts = last.split(":");
                        if (parts.length == 2) {
                            String p = "/storage/emulated/0/" + parts[1];
                            if (new File(p).exists()) return p;
                        }
                    }
                }
            }
        } catch (Exception ignored) {}
        return null;
    }

    public static String copyUriToTempFile(Context context, Uri uri, String prefix, String suffix) {
        try {
            File tempFile = File.createTempFile(prefix, suffix, context.getCacheDir());
            tempFile.deleteOnExit();
            try (InputStream is = context.getContentResolver().openInputStream(uri);
                 OutputStream os = new FileOutputStream(tempFile)) {
                if (is == null) return null;
                byte[] buffer = new byte[8192];
                int read;
                while ((read = is.read(buffer)) != -1) {
                    os.write(buffer, 0, read);
                }
                os.flush();
            }
            return tempFile.getAbsolutePath();
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    public static boolean copyFileToUri(Context context, File sourceFile, Uri destUri) {
        try (InputStream is = new java.io.FileInputStream(sourceFile);
             OutputStream os = context.getContentResolver().openOutputStream(destUri)) {
            if (os == null) return false;
            byte[] buffer = new byte[8192];
            int read;
            while ((read = is.read(buffer)) != -1) {
                os.write(buffer, 0, read);
            }
            os.flush();
            return true;
        } catch (Exception e) {
            e.printStackTrace();
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

    /**
     * Convert a virtual disk image format asynchronously using qemu-img.
     *
     * @param context      Android Context
     * @param sourceUri    Source Uri (file:// or content://)
     * @param destUri      Destination Uri (file:// or content://)
     * @param targetFormat Target format ("qcow2", "raw", "vmdk", "vdi")
     * @param cb           Callback for result
     */
    public static void convertDiskAsync(
            Context context, Uri sourceUri, Uri destUri, String targetFormat,
            DiskConvertCallback cb) {

        new AsyncTask<Void, String, String>() {
            private String resolvedDestPath = null;

            @Override protected void onPreExecute() {
                cb.onProgress("Converting disk to " + targetFormat.toUpperCase() + "…");
            }

            @Override protected String doInBackground(Void... v) {
                File tempSourceFile = null;
                File tempDestFile = null;
                try {
                    // Resolve source path
                    String srcPath = resolveUri(context, sourceUri);
                    if (srcPath == null || !new File(srcPath).exists()) {
                        publishProgress("Copying source content URI to temp file…");
                        srcPath = copyUriToTempFile(context, sourceUri, "convert_src_", ".tmp");
                        if (srcPath != null) {
                            tempSourceFile = new File(srcPath);
                        } else {
                            return "Unable to access source disk image.";
                        }
                    }

                    // Resolve dest path
                    String dstPath = resolveUri(context, destUri);
                    boolean directDest = false;
                    if (dstPath != null) {
                        File parent = new File(dstPath).getParentFile();
                        if (parent != null && (parent.exists() || parent.mkdirs())) {
                            directDest = true;
                        }
                    }

                    if (!directDest) {
                        tempDestFile = File.createTempFile("convert_dst_", "." + targetFormat, context.getCacheDir());
                        tempDestFile.deleteOnExit();
                        dstPath = tempDestFile.getAbsolutePath();
                    }
                    resolvedDestPath = (dstPath != null && directDest) ? dstPath : destUri.toString();

                    String qemuImg = resolveQemuImg();
                    List<String> cmd = new ArrayList<>();
                    cmd.add(qemuImg);
                    cmd.add("convert");
                    cmd.add("-O"); cmd.add(targetFormat);
                    cmd.add(srcPath);
                    cmd.add(dstPath);

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

                    if (proc.exitValue() != 0) {
                        return "qemu-img convert failed (exit code " + proc.exitValue() + "):\n" + out;
                    }

                    // If we converted to a temp dest file, copy back to destUri
                    if (!directDest && tempDestFile != null) {
                        publishProgress("Writing converted image to destination…");
                        boolean copied = copyFileToUri(context, tempDestFile, destUri);
                        if (!copied) {
                            return "Failed to write converted disk image to destination URI.";
                        }
                    }

                    return null; // success
                } catch (Exception e) {
                    return "Disk conversion error: " + e.getMessage();
                } finally {
                    if (tempSourceFile != null && tempSourceFile.exists()) {
                        try { tempSourceFile.delete(); } catch (Exception ignored) {}
                    }
                    if (tempDestFile != null && tempDestFile.exists()) {
                        try { tempDestFile.delete(); } catch (Exception ignored) {}
                    }
                }
            }

            @Override protected void onProgressUpdate(String... v) {
                cb.onProgress(v[0]);
            }

            @Override protected void onPostExecute(String err) {
                if (err == null) cb.onSuccess(resolvedDestPath);
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

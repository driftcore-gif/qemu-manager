package com.qemumanager.adapter;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.qemumanager.R;
import com.qemumanager.model.VMConfig;
import java.util.List;

public class VMAdapter extends RecyclerView.Adapter<VMAdapter.VH> {

    public interface OnClickListener { void onClick(VMConfig vm); }

    private final List<VMConfig>  vms;
    private final OnClickListener listener;

    public VMAdapter(List<VMConfig> vms, OnClickListener listener) {
        this.vms = vms;
        this.listener = listener;
    }

    @NonNull @Override
    public VH onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View v = LayoutInflater.from(parent.getContext())
            .inflate(R.layout.item_vm, parent, false);
        return new VH(v);
    }

    @Override
    public void onBindViewHolder(@NonNull VH h, int pos) {
        VMConfig vm = vms.get(pos);
        h.tvName.setText(vm.name);
        h.tvArch.setText(vm.arch.name() + " · " + vm.ramMb + " MB · " + vm.accel.name());
        h.tvStatus.setText(vm.status.name());
        h.tvStatus.setTextColor(vm.status == VMConfig.VMStatus.Running
            ? 0xFF4CAF50 : 0xFFBDBDBD);
        h.itemView.setOnClickListener(v -> listener.onClick(vm));
    }

    @Override public int getItemCount() { return vms.size(); }

    static class VH extends RecyclerView.ViewHolder {
        TextView tvName, tvArch, tvStatus;
        VH(View v) {
            super(v);
            tvName   = v.findViewById(R.id.tv_vm_name);
            tvArch   = v.findViewById(R.id.tv_vm_arch);
            tvStatus = v.findViewById(R.id.tv_vm_status);
        }
    }
}

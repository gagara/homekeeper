package com.gagara.homekeeper.activity;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.Spinner;
import android.widget.ToggleButton;

import com.gagara.homekeeper.R;

public class ManageNodeStateDialog extends DialogFragment {

    private SwitchNodeStateListener listener = null;
    private int nodeId = 0;
    private String nodeName = "";
    private boolean manualMode = true;
    private boolean state = true;
    private int period = 0;

    private CheckBox modeToggle;
    private ToggleButton stateToggle;
    private Spinner periodSpinner;

    public int getNodeId() {
        return nodeId;
    }

    public void setNodeId(int nodeId) {
        this.nodeId = nodeId;
    }

    public void setNodeName(String nodeName) {
        this.nodeName = nodeName;
    }

    public boolean isManualMode() {
        return manualMode;
    }

    public void setManualMode(boolean manualMode) {
        this.manualMode = manualMode;
    }

    public void setState(boolean state) {
        this.state = state;
    }

    public boolean getState() {
        return state;
    }

    public int getPeriod() {
        return period;
    }

    public void setPeriod(int period) {
        this.period = period;
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        if (activity instanceof SwitchNodeStateListener) {
            listener = (SwitchNodeStateListener) activity;
        }
    }

    @SuppressLint("InflateParams")
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setMessage(String.format(getResources().getString(
                R.string.manage_node_dialog_title, nodeName)));

        LayoutInflater inflater = getActivity().getLayoutInflater();
        View view = inflater.inflate(R.layout.manage_node_state, null);

        modeToggle = (CheckBox) view.findViewById(R.id.modeFlag);
        modeToggle.setChecked(manualMode);
        modeToggle.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    stateToggle.setEnabled(true);
                    periodSpinner.setEnabled(true);
                } else {
                    stateToggle.setEnabled(false);
                    periodSpinner.setEnabled(false);
                }
            }
        });

        stateToggle = (ToggleButton) view.findViewById(R.id.nodeStateSwitch);
        stateToggle.setChecked(state);

        periodSpinner = (Spinner) view.findViewById(R.id.periodList);
        int[] periodOptions = getResources().getIntArray(R.array.node_switch_period_entry_values);
        for (int i = 0; i < periodOptions.length; i++) {
            if (periodOptions[i] == period) {
                periodSpinner.setSelection(i);
                break;
            }
        }
        if (!manualMode) {
            stateToggle.setEnabled(false);
            periodSpinner.setEnabled(false);
        }

        builder.setView(view);

        builder.setPositiveButton(R.string.manage_node_button_ok,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        readValues();
                        if (listener != null) {
                            listener.doSwitchNodeState(ManageNodeStateDialog.this);
                        }
                    }
                });
        builder.setNegativeButton(R.string.manage_node_button_cancel,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        readValues();
                        if (listener != null) {
                            listener.doNotSwitchNodeState(ManageNodeStateDialog.this);
                        }
                    }
                });
        return builder.create();
    }

    @Override
    public void onSaveInstanceState(Bundle bundle) {
        super.onSaveInstanceState(bundle);
        bundle.putInt("nodeId", nodeId);
        bundle.putString("nodeName", nodeName);
    }

    @Override
    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        if (bundle != null) {
            if (bundle.containsKey("nodeId")) {
                nodeId = bundle.getInt("nodeId");
            }
            if (bundle.containsKey("nodeName")) {
                nodeName = bundle.getString("nodeName");
            }
        }
    }

    private void readValues() {
        manualMode = modeToggle.isChecked();
        state = stateToggle.isChecked();
        period = getResources().getIntArray(R.array.node_switch_period_entry_values)[periodSpinner
                .getSelectedItemPosition()];
    }

    public interface SwitchNodeStateListener {

        public void doSwitchNodeState(ManageNodeStateDialog dialog);

        public void doNotSwitchNodeState(ManageNodeStateDialog dialog);

    }

}

package com.gagara.homekeeper.activity;

import static com.gagara.homekeeper.ui.viewmodel.TopModelView.SENSORS_THRESHOLDS;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.text.InputType;
import android.util.SparseArray;
import android.util.SparseIntArray;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TableLayout;
import android.widget.TextView;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.SensorModel;

public class ConfigureNodeDialog extends DialogFragment {

    private View view = null;

    private ConfigureNodeListener listener = null;
    private int nodeId = 0;
    private String nodeName = "";
    private SparseIntArray result = new SparseIntArray();

    private SparseArray<SensorModel> sensorsThresholds;

    public int getNodeId() {
        return nodeId;
    }

    public void setNodeId(int nodeId) {
        this.nodeId = nodeId;
    }

    public void setNodeName(String nodeName) {
        this.nodeName = nodeName;
    }

    public void setSensorsThresholds(SparseArray<SensorModel> sensorsThresholds) {
        this.sensorsThresholds = sensorsThresholds;
    }

    public SparseIntArray getResult() {
        return result;
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        if (activity instanceof ConfigureNodeListener) {
            listener = (ConfigureNodeListener) activity;
        }
    }

    @SuppressLint({ "InlinedApi", "InflateParams" })
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setMessage(String.format(getResources().getString(R.string.configure_node_dialog_title, nodeName)));

        LayoutInflater inflater = getActivity().getLayoutInflater();
        view = inflater.inflate(R.layout.configure_node, null);
        ViewGroup insertPoint = (ViewGroup) view.findViewById(R.id.configNode);

        for (int i = 0; i < sensorsThresholds.size(); i++) {
            // outer container
            LinearLayout ll = new LinearLayout(this.getActivity());
            ll.setLayoutParams(new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));
            ll.setOrientation(LinearLayout.HORIZONTAL);

            // title
            TextView title = new TextView(this.getActivity());
            title.setLayoutParams(new TableLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
            title.setGravity(Gravity.START);
            title.setText(SENSORS_THRESHOLDS.get(sensorsThresholds.valueAt(i).getId()));

            // control
            EditText control = new EditText(this.getActivity());
            control.setId(sensorsThresholds.valueAt(i).getId());
            control.setLayoutParams(new TableLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
            control.setGravity(Gravity.START);
            control.setText(sensorsThresholds.valueAt(i).getValue() + "");
            control.setInputType(InputType.TYPE_CLASS_NUMBER);

            ll.addView(title);
            ll.addView(control);
            insertPoint.addView(ll, 0);
        }

        builder.setView(view);

        builder.setPositiveButton(R.string.configure_node_button_ok, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                try {
                    readValues();
                    if (listener != null) {
                        listener.doConfigure(ConfigureNodeDialog.this);
                    }
                } catch (NumberFormatException e) {
                    // invalid value. just ignore it
                }
            }
        });
        builder.setNegativeButton(R.string.configure_node_button_cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                try {
                    readValues();
                    if (listener != null) {
                        listener.doNotConfigure(ConfigureNodeDialog.this);
                    }
                } catch (NumberFormatException e) {
                    // invalid value. just ignore it
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
        if (view != null) {
            for (int i = 0; i < sensorsThresholds.size(); i++) {
                SensorModel sensor = sensorsThresholds.valueAt(i);
                result.put(sensor.getId(),
                        Integer.parseInt(((EditText) view.findViewById(sensor.getId())).getText().toString()));
            }
        }
    }

    public interface ConfigureNodeListener {

        public void doConfigure(ConfigureNodeDialog dialog);

        public void doNotConfigure(ConfigureNodeDialog dialog);

    }

}

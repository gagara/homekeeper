package com.gagara.homekeeper.activity;

import static android.widget.Toast.LENGTH_LONG;
import static com.gagara.homekeeper.common.Constants.COMMAND_KEY;
import static com.gagara.homekeeper.common.Constants.CONTROLLER_CONTROL_COMMAND_ACTION;
import static com.gagara.homekeeper.common.Constants.DEFAULT_SWITCH_OFF_PERIOD_SEC;
import static com.gagara.homekeeper.common.Constants.DEFAULT_SWITCH_ON_PERIOD_SEC;
import static com.gagara.homekeeper.common.Constants.REQUEST_ENABLE_BT;

import java.util.Map.Entry;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Parcelable;
import android.support.v4.app.DialogFragment;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.app.ActionBarActivity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Toast;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.activity.ManageNodeStateDialog.SwitchNodeStateListener;
import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.Mode;
import com.gagara.homekeeper.model.NodeModel;
import com.gagara.homekeeper.model.SensorModel;
import com.gagara.homekeeper.nbi.request.CurrentStatusRequest;
import com.gagara.homekeeper.nbi.request.NodeStateChangeRequest;
import com.gagara.homekeeper.nbi.response.CurrentStatusResponse;
import com.gagara.homekeeper.nbi.response.NodeStateChangeResponse;
import com.gagara.homekeeper.service.BtCommunicationService;
import com.gagara.homekeeper.ui.viewmodel.NodeModelView;
import com.gagara.homekeeper.ui.viewmodel.TopModelView;
import com.gagara.homekeeper.utils.BluetoothUtils;
import com.gagara.homekeeper.utils.HomeKeeperConfig;

public class MainActivity extends ActionBarActivity implements SwitchNodeStateListener {

    private BluetoothDevice activeDev = null;

    private BroadcastReceiver btStateChangedReceiver;
    private BtServiceDataReceiver dataReceiver;
    private BtServiceStatusReceiver serviceStatusReceiver;

    private TopModelView modelView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        modelView = new TopModelView(this);
        modelView.render();

        btStateChangedReceiver = new BroadcastReceiver() {
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)) {
                    int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE,
                            BluetoothAdapter.ERROR);
                    switch (state) {
                    case BluetoothAdapter.STATE_OFF:
                        stopControllerCommunicationService();
                        break;
                    case BluetoothAdapter.STATE_ON:
                        if (HomeKeeperConfig.getMode(MainActivity.this) == Mode.MASTER) {
                            startControllerCommunicationService();
                        }
                        break;
                    }
                }
            }
        };
        dataReceiver = new BtServiceDataReceiver();
        serviceStatusReceiver = new BtServiceStatusReceiver();
    }

    @Override
    protected void onResume() {
        super.onResume();

        registerReceiver(btStateChangedReceiver, new IntentFilter(
                BluetoothAdapter.ACTION_STATE_CHANGED));
        LocalBroadcastManager.getInstance(this).registerReceiver(dataReceiver,
                new IntentFilter(Constants.CONTROLLER_DATA_TRANSFER_ACTION));
        LocalBroadcastManager.getInstance(this).registerReceiver(serviceStatusReceiver,
                new IntentFilter(Constants.BT_SERVICE_STATUS_ACTION));

        initControllerService();
    }

    @Override
    protected void onStop() {
        super.onStop();
        unregisterReceiver(btStateChangedReceiver);
        LocalBroadcastManager.getInstance(this).unregisterReceiver(dataReceiver);
        LocalBroadcastManager.getInstance(this).unregisterReceiver(serviceStatusReceiver);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.action_settings) {
            Intent intent = new Intent(this, SettingsActivity.class);
            startActivity(intent);
            return true;
        } else if (id == R.id.action_refresh) {
            CurrentStatusRequest request = new CurrentStatusRequest();
            if (BluetoothUtils.isEnabled()) {
                request.setSrcAddress(BluetoothUtils.getAdapter().getAddress());
            }
            Intent intent = new Intent();
            intent.setAction(CONTROLLER_CONTROL_COMMAND_ACTION);
            intent.putExtra(COMMAND_KEY, new CurrentStatusRequest());
            LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
            return true;
        } else if (id == R.id.action_exit) {
            stopControllerCommunicationService();
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onSaveInstanceState(Bundle bundle) {
        super.onSaveInstanceState(bundle);
        modelView.saveState(bundle);
    }

    @Override
    protected void onRestoreInstanceState(Bundle bundle) {
        super.onRestoreInstanceState(bundle);
        modelView.restoreState(bundle);
        modelView.render();
    }

    public void onManageNode(View nodeToggle) {
        NodeModelView nodeView = null;
        for (Entry<Integer, Integer> e : TopModelView.NODES_VALUE_VIEW_MAP.entrySet()) {
            if (e.getValue() == nodeToggle.getId()) {
                nodeView = modelView.getNode(e.getKey());
                break;
            }
        }
        if (nodeView != null) {
            ManageNodeStateDialog dialog = new ManageNodeStateDialog();
            dialog.setNodeId(nodeView.getModel().getId());
            dialog.setNodeName(getResources().getString(
                    TopModelView.NODES_NAME_VIEW_MAP.get(nodeView.getModel().getId())));
            dialog.setManualMode(!nodeView.getModel().isForcedMode());
            dialog.setState(!nodeView.getModel().getState());
            dialog.setPeriod(nodeView.getModel().getState() ? DEFAULT_SWITCH_OFF_PERIOD_SEC
                    : DEFAULT_SWITCH_ON_PERIOD_SEC);
            dialog.show(getSupportFragmentManager(), Constants.SWITCH_NODE_DIALOG_TAG);
        }
        nodeView.render();
    }

    @Override
    public void doSwitchNodeState(DialogFragment dialog) {
        ManageNodeStateDialog manageDialog = (ManageNodeStateDialog) dialog;
        NodeStateChangeRequest request = new NodeStateChangeRequest();
        if (BluetoothUtils.isEnabled()) {
            request.setSrcAddress(BluetoothUtils.getAdapter().getAddress());
        }

        request.setId(manageDialog.getNodeId());
        if (manageDialog.isManualMode()) {
            request.setState(manageDialog.getState());
            if (manageDialog.getPeriod() != 0) {
                request.setPeriod(manageDialog.getPeriod() * 1000L);
            }
        }

        Intent intent = new Intent();
        intent.setAction(CONTROLLER_CONTROL_COMMAND_ACTION);
        intent.putExtra(COMMAND_KEY, request);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    @Override
    public void doNotSwitchNodeState(DialogFragment dialog) {
        // do nothing
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (REQUEST_ENABLE_BT == requestCode) {
            if (RESULT_OK == resultCode) {
                startControllerCommunicationService();
            } else {
                Toast.makeText(this, R.string.bt_disabled_error, LENGTH_LONG).show();
                finish();
            }
        } else {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    private void initControllerService() {
        if (HomeKeeperConfig.getMode(this, null) == Mode.MASTER) {
            if (!BluetoothUtils.enableBluetooth(this)) {
                Toast.makeText(this, R.string.bt_not_supported_error, LENGTH_LONG).show();
                finish();
            }
            if (BluetoothUtils.isEnabled()) {
                startControllerCommunicationService();
            }
        } else {
            stopControllerCommunicationService();
        }
    }

    private void startControllerCommunicationService() {
        if (activeDev != null && !activeDev.equals(HomeKeeperConfig.getBluetoothDevice(this))) {
            stopControllerCommunicationService();
        }
        if (HomeKeeperConfig.getBluetoothDevice(this) != null) {
            startService(new Intent(this, BtCommunicationService.class));
            activeDev = HomeKeeperConfig.getBluetoothDevice(this);
            modelView.getTitle().getModel()
                    .setName(HomeKeeperConfig.getBluetoothDevice(this).getName());
        } else {
            modelView.getTitle().getModel().setName(null);
        }
        modelView.getTitle().getModel().setMode(HomeKeeperConfig.getMode(this));
        modelView.getTitle().render();
        modelView.getInfo().render();
    }

    private void stopControllerCommunicationService() {
        stopService(new Intent(this, BtCommunicationService.class));
        activeDev = null;
    }

    public class BtServiceDataReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Constants.CONTROLLER_DATA_TRANSFER_ACTION.equals(action)) {
                Parcelable data = intent.getParcelableExtra(Constants.DATA_KEY);
                if (data instanceof CurrentStatusResponse) {
                    CurrentStatusResponse stats = (CurrentStatusResponse) data;
                    modelView.getInfo().getModel().setTimestamp(stats.getTimestamp());
                    modelView.getInfo().render();

                    // sensors
                    for (int i = 0; i < stats.getSensors().size(); i++) {
                        SensorModel sensor = stats.getSensors().valueAt(i).getData();
                        modelView.getSensor(sensor.getId()).getModel().update(sensor);
                        modelView.getSensor(sensor.getId()).render();
                    }

                    // nodes
                    for (int i = 0; i < stats.getNodes().size(); i++) {
                        NodeModel node = stats.getNodes().valueAt(i).getData();
                        modelView.getNode(node.getId()).getModel().update(node);
                        modelView.getNode(node.getId()).render();
                    }

                    // Toast.makeText(MainActivity.this,
                    // R.string.updating_message, LENGTH_LONG).show();
                } else if (data instanceof NodeStateChangeResponse) {
                    // node
                    NodeStateChangeResponse stats = (NodeStateChangeResponse) data;
                    NodeModel node = stats.getData();
                    modelView.getNode(node.getId()).setModel(node);
                    modelView.getNode(node.getId()).render();

                    String msg = null;
                    if (node.getState()) {
                        msg = String.format(getResources().getString(
                                R.string.switch_state_on_message,
                                getResources().getString(
                                        TopModelView.NODES_NAME_VIEW_MAP.get(node.getId()))));
                    } else {
                        msg = String.format(getResources().getString(
                                R.string.switch_state_off_message,
                                getResources().getString(
                                        TopModelView.NODES_NAME_VIEW_MAP.get(node.getId()))));
                    }
                    Toast.makeText(MainActivity.this, msg, LENGTH_LONG).show();
                } else {
                    // unknown data. ignoring
                }
            }
        }
    }

    public class BtServiceStatusReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Constants.BT_SERVICE_STATUS_ACTION.equals(action)) {
                modelView
                        .getStatus()
                        .getModel()
                        .setState(
                                BtCommunicationService.State.fromString(intent
                                        .getStringExtra(Constants.SERVICE_STATUS_KEY)));
                modelView.getStatus().getModel()
                        .setDetails(intent.getStringExtra(Constants.SERVICE_STATUS_DETAILS_KEY));

                modelView.getStatus().render();
            }
        }
    }
}

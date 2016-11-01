package com.gagara.homekeeper.activity;

import static android.widget.Toast.LENGTH_LONG;
import static com.gagara.homekeeper.common.Constants.COMMAND_KEY;
import static com.gagara.homekeeper.common.Constants.CONTROLLER_CONTROL_COMMAND_ACTION;
import static com.gagara.homekeeper.common.Constants.DEFAULT_SWITCH_OFF_PERIOD_SEC;
import static com.gagara.homekeeper.common.Constants.DEFAULT_SWITCH_ON_PERIOD_SEC;
import static com.gagara.homekeeper.common.Constants.REQUEST_ENABLE_BT;
import static com.gagara.homekeeper.common.Constants.REQUEST_ENABLE_NETWORK;
import static com.gagara.homekeeper.common.Constants.SERVICE_STATUS_CHANGE_ACTION;
import static com.gagara.homekeeper.common.Constants.SERVICE_STATUS_DETAILS_KEY;
import static com.gagara.homekeeper.common.Constants.SERVICE_STATUS_KEY;
import static com.gagara.homekeeper.common.Constants.SERVICE_TITLE_CHANGE_ACTION;
import static com.gagara.homekeeper.common.Constants.SERVICE_TITLE_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_SB_HEATER_ID;

import java.util.Date;
import java.util.Map.Entry;

import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.os.Bundle;
import android.os.Parcelable;
import android.support.v4.app.DialogFragment;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.app.ActionBarActivity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.animation.AnimationUtils;
import android.widget.Toast;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.activity.ManageNodeStateDialog.SwitchNodeStateListener;
import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.Mode;
import com.gagara.homekeeper.common.Proxy;
import com.gagara.homekeeper.model.NodeModel;
import com.gagara.homekeeper.model.SensorModel;
import com.gagara.homekeeper.model.ServiceStatusModel;
import com.gagara.homekeeper.nbi.request.CurrentStatusRequest;
import com.gagara.homekeeper.nbi.request.NodeStateChangeRequest;
import com.gagara.homekeeper.nbi.response.CurrentStatusResponse;
import com.gagara.homekeeper.nbi.response.NodeStateChangeResponse;
import com.gagara.homekeeper.nbi.response.SensorThresholdConfigurationResponse;
import com.gagara.homekeeper.nbi.service.BluetoothNbiService;
import com.gagara.homekeeper.nbi.service.ProxyNbiService;
import com.gagara.homekeeper.nbi.service.ServiceState;
import com.gagara.homekeeper.ui.view.ViewUtils;
import com.gagara.homekeeper.ui.viewmodel.NodeModelView;
import com.gagara.homekeeper.ui.viewmodel.TopModelView;
import com.gagara.homekeeper.utils.BluetoothUtils;
import com.gagara.homekeeper.utils.HomeKeeperConfig;
import com.gagara.homekeeper.utils.NetworkUtils;

public class MainActivity extends ActionBarActivity implements SwitchNodeStateListener {

    private BroadcastReceiver btStateChangedReceiver;
    private BroadcastReceiver proxyStateChangedReceiver;
    private BroadcastReceiver dataReceiver;
    private BroadcastReceiver serviceTitleReceiver;
    private BroadcastReceiver serviceStatusReceiver;

    private TopModelView modelView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        modelView = new TopModelView(this);
        modelView.render();

        btStateChangedReceiver = new BluetoothStateChangedReceiver();
        proxyStateChangedReceiver = new NetworkStateChangedReceiver();
        dataReceiver = new NbiServiceDataReceiver();
        serviceTitleReceiver = new NbiServiceTitleReceiver();
        serviceStatusReceiver = new NbiServiceStatusReceiver();
    }

    @Override
    protected void onResume() {
        super.onResume();

        registerReceiver(btStateChangedReceiver, new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED));
        registerReceiver(proxyStateChangedReceiver, new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION));

        LocalBroadcastManager.getInstance(this).registerReceiver(dataReceiver,
                new IntentFilter(Constants.CONTROLLER_DATA_TRANSFER_ACTION));
        LocalBroadcastManager.getInstance(this).registerReceiver(serviceTitleReceiver,
                new IntentFilter(SERVICE_TITLE_CHANGE_ACTION));
        LocalBroadcastManager.getInstance(this).registerReceiver(serviceStatusReceiver,
                new IntentFilter(SERVICE_STATUS_CHANGE_ACTION));

        startBluetoothNbiService();
        startProxyNbiService();
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(btStateChangedReceiver);
        unregisterReceiver(proxyStateChangedReceiver);
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
            stopBluetoothNbiService();
            stopProxyNbiService();
            Intent intent = new Intent(this, SettingsActivity.class);
            startActivity(intent);
            return true;
        } else if (id == R.id.action_refresh) {
            Intent intent = new Intent();
            intent.setAction(CONTROLLER_CONTROL_COMMAND_ACTION);
            intent.putExtra(COMMAND_KEY, new CurrentStatusRequest());
            LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
            return true;
        } else if (id == R.id.action_exit) {
            stopBluetoothNbiService();
            stopProxyNbiService();
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
                startBluetoothNbiService();
            } else {
                Toast.makeText(this, R.string.bt_disabled_error, LENGTH_LONG).show();
            }
        } else if (REQUEST_ENABLE_NETWORK == requestCode) {
            startProxyNbiService();
        } else {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    private synchronized void startBluetoothNbiService() {
        if (HomeKeeperConfig.getMode(this, null) == Mode.DIRECT) {
            if (BluetoothUtils.isEnabled()) {
                if (HomeKeeperConfig.getBluetoothDevice(this) != null) {
                    startService(new Intent(this, BluetoothNbiService.class));
                } else {
                    Intent intent = new Intent(SERVICE_STATUS_CHANGE_ACTION);
                    intent.putExtra(SERVICE_STATUS_DETAILS_KEY,
                            getResources().getString(R.string.bt_not_configured_error));
                    LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
                }
            } else {
                Intent intent = new Intent(SERVICE_STATUS_CHANGE_ACTION);
                intent.putExtra(SERVICE_STATUS_DETAILS_KEY, getResources().getString(R.string.bt_disabled_error));
                LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
            }
        }
    }

    private synchronized void startProxyNbiService() {
        if (HomeKeeperConfig.getMode(this, null) == Mode.PROXY) {
            Proxy proxy = HomeKeeperConfig.getNbiProxy(this);
            if (NetworkUtils.isEnabled(this)) {
                if (proxy != null && proxy.valid()) {
                    startService(new Intent(this, ProxyNbiService.class));
                } else {
                    Intent intent = new Intent(SERVICE_STATUS_CHANGE_ACTION);
                    intent.putExtra(SERVICE_STATUS_DETAILS_KEY,
                            getResources().getString(R.string.proxy_not_configured_error));
                    LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
                }
            } else {
                Intent intent = new Intent(SERVICE_STATUS_CHANGE_ACTION);
                intent.putExtra(SERVICE_STATUS_DETAILS_KEY, getResources().getString(R.string.networks_disabled_error));
                LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
            }
        }
    }

    private void stopBluetoothNbiService() {
        stopService(new Intent(this, BluetoothNbiService.class));
    }

    private void stopProxyNbiService() {
        stopService(new Intent(this, ProxyNbiService.class));
    }

    public class NbiServiceDataReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Constants.CONTROLLER_DATA_TRANSFER_ACTION.equals(action)) {
                Parcelable data = intent.getParcelableExtra(Constants.DATA_KEY);
                Date latestTimestamp = null;
                if (data instanceof CurrentStatusResponse) {
                    CurrentStatusResponse stats = (CurrentStatusResponse) data;

                    // sensor
                    if (stats.getSensor() != null) {
                        SensorModel sensor = stats.getSensor().getData();
                        if (ViewUtils.validSensor(sensor)) {
                            modelView.getSensor(sensor.getId()).getModel().update(sensor);
                            modelView.getSensor(sensor.getId()).render();
                            latestTimestamp = stats.getTimestamp();
                        }
                    }
                    // node
                    if (stats.getNode() != null) {
                        NodeModel node = stats.getNode().getData();
                        if (ViewUtils.validNode(node)) {
                            modelView.getNode(node.getId()).getModel().update(node);
                            modelView.getNode(node.getId()).render();
                            latestTimestamp = stats.getTimestamp();
                        }
                    }

                } else if (data instanceof NodeStateChangeResponse) {
                    // node
                    NodeStateChangeResponse stats = (NodeStateChangeResponse) data;
                    NodeModel node = stats.getData();
                    if (ViewUtils.validNode(node)) {
                        modelView.getNode(node.getId()).setModel(node);
                        modelView.getNode(node.getId()).render();
                        for (int i = 0; i < node.getSensors().size(); i++) {
                            SensorModel sensor = node.getSensors().valueAt(i);
                            if (ViewUtils.validSensor(sensor)) {
                                modelView.getSensor(sensor.getId()).getModel().update(sensor);
                                modelView.getSensor(sensor.getId()).render();
                            }
                        }
                        latestTimestamp = stats.getTimestamp();

                        String msg = null;
                        if (node.getState()) {
                            msg = String.format(getResources().getString(R.string.switch_state_on_message,
                                    getResources().getString(TopModelView.NODES_NAME_VIEW_MAP.get(node.getId()))));
                        } else {
                            msg = String.format(getResources().getString(R.string.switch_state_off_message,
                                    getResources().getString(TopModelView.NODES_NAME_VIEW_MAP.get(node.getId()))));
                        }
                        Toast.makeText(MainActivity.this, msg, LENGTH_LONG).show();
                    }
                } else if (data instanceof SensorThresholdConfigurationResponse) {
                    SensorThresholdConfigurationResponse cfg = (SensorThresholdConfigurationResponse) data;
                    if (cfg.getData() != null) {
                        SensorModel sensor = cfg.getData();
                        // hardcoded for now
                        int nodeId = NODE_SB_HEATER_ID;
                        if (ViewUtils.validSensor(sensor)) {
                            modelView.getNode(nodeId).getModel().addSensorThreshold(sensor);
                            modelView.getNode(nodeId).render();
                            latestTimestamp = cfg.getTimestamp();
                        }
                    }
                } else {
                    // unknown data. ignoring
                }
                if (latestTimestamp != null) {
                    modelView.getInfo().getModel().setTimestamp(latestTimestamp);
                    modelView.getInfo().render();
                }
            }
        }
    }

    public class NbiServiceTitleReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (SERVICE_TITLE_CHANGE_ACTION.equals(action)) {
                modelView.getTitle().getModel().setMode(HomeKeeperConfig.getMode(context, null));
                modelView.getTitle().getModel().setName(intent.getStringExtra(SERVICE_TITLE_KEY));

                modelView.getTitle().render();
            }
        }
    }

    public class NbiServiceStatusReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (SERVICE_STATUS_CHANGE_ACTION.equals(action)) {
                ServiceStatusModel status = new ServiceStatusModel();
                status.setMode(HomeKeeperConfig.getMode(context, null));
                status.setState(ServiceState.fromString(intent.getStringExtra(SERVICE_STATUS_KEY)));
                status.setDetails(intent.getStringExtra(SERVICE_STATUS_DETAILS_KEY));
                status.setTimestamp(new Date());
                modelView.getLog().getModel().add(status);
                modelView.getLog().getView()
                        .setAnimation(AnimationUtils.loadAnimation(context, R.anim.abc_slide_in_bottom));

                modelView.getLog().render();
            }
        }
    }

    private class BluetoothStateChangedReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)
                    && HomeKeeperConfig.getMode(MainActivity.this, null) == Mode.DIRECT) {
                int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
                switch (state) {
                case BluetoothAdapter.STATE_OFF:
                    stopBluetoothNbiService();
                    break;
                case BluetoothAdapter.STATE_ON:
                    startBluetoothNbiService();
                    break;
                }
            }
        }
    }

    private class NetworkStateChangedReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ConnectivityManager.CONNECTIVITY_ACTION.equals(action)
                    && HomeKeeperConfig.getMode(MainActivity.this, null) == Mode.PROXY) {
                if (NetworkUtils.isEnabled(context)) {
                    startProxyNbiService();
                } else {
                    stopProxyNbiService();
                }
            }
        }
    }
}

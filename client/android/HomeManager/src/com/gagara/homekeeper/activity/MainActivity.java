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
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
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
import com.gagara.homekeeper.common.Proxy;
import com.gagara.homekeeper.model.NodeModel;
import com.gagara.homekeeper.model.SensorModel;
import com.gagara.homekeeper.nbi.request.CurrentStatusRequest;
import com.gagara.homekeeper.nbi.request.NodeStateChangeRequest;
import com.gagara.homekeeper.nbi.response.CurrentStatusResponse;
import com.gagara.homekeeper.nbi.response.NodeStateChangeResponse;
import com.gagara.homekeeper.nbi.service.BluetoothNbiService;
import com.gagara.homekeeper.nbi.service.ProxyNbiService;
import com.gagara.homekeeper.nbi.service.ServiceState;
import com.gagara.homekeeper.ui.view.ViewUtils;
import com.gagara.homekeeper.ui.viewmodel.NodeModelView;
import com.gagara.homekeeper.ui.viewmodel.TopModelView;
import com.gagara.homekeeper.utils.BluetoothUtils;
import com.gagara.homekeeper.utils.HomeKeeperConfig;

public class MainActivity extends ActionBarActivity implements
        SwitchNodeStateListener {

    private BluetoothDevice activeDev = null;
    private Proxy activeProxy = null;

    private BroadcastReceiver btStateChangedReceiver;
    private BroadcastReceiver proxyStateChangedReceiver;
    private NbiServiceDataReceiver dataReceiver;
    private NbiServiceStatusReceiver serviceStatusReceiver;

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
        serviceStatusReceiver = new NbiServiceStatusReceiver();
    }

    @Override
    protected void onResume() {
        super.onResume();

        registerReceiver(btStateChangedReceiver, new IntentFilter(
                BluetoothAdapter.ACTION_STATE_CHANGED));
        registerReceiver(proxyStateChangedReceiver, new IntentFilter(
                ConnectivityManager.CONNECTIVITY_ACTION));
        LocalBroadcastManager.getInstance(this).registerReceiver(dataReceiver,
                new IntentFilter(Constants.CONTROLLER_DATA_TRANSFER_ACTION));
        LocalBroadcastManager.getInstance(this).registerReceiver(
                serviceStatusReceiver,
                new IntentFilter(Constants.SERVICE_STATUS_ACTION));

        initNbiService();
    }

    @Override
    protected void onStop() {
        super.onStop();
        unregisterReceiver(btStateChangedReceiver);
        unregisterReceiver(proxyStateChangedReceiver);
        LocalBroadcastManager.getInstance(this)
                .unregisterReceiver(dataReceiver);
        LocalBroadcastManager.getInstance(this).unregisterReceiver(
                serviceStatusReceiver);
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
            Intent intent = new Intent();
            intent.setAction(CONTROLLER_CONTROL_COMMAND_ACTION);
            intent.putExtra(COMMAND_KEY, new CurrentStatusRequest());
            LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
            return true;
        } else if (id == R.id.action_exit) {
            stopBluetoothNbiService();
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
        for (Entry<Integer, Integer> e : TopModelView.NODES_VALUE_VIEW_MAP
                .entrySet()) {
            if (e.getValue() == nodeToggle.getId()) {
                nodeView = modelView.getNode(e.getKey());
                break;
            }
        }
        if (nodeView != null) {
            ManageNodeStateDialog dialog = new ManageNodeStateDialog();
            dialog.setNodeId(nodeView.getModel().getId());
            dialog.setNodeName(getResources().getString(
                    TopModelView.NODES_NAME_VIEW_MAP.get(nodeView.getModel()
                            .getId())));
            dialog.setManualMode(!nodeView.getModel().isForcedMode());
            dialog.setState(!nodeView.getModel().getState());
            dialog.setPeriod(nodeView.getModel().getState() ? DEFAULT_SWITCH_OFF_PERIOD_SEC
                    : DEFAULT_SWITCH_ON_PERIOD_SEC);
            dialog.show(getSupportFragmentManager(),
                    Constants.SWITCH_NODE_DIALOG_TAG);
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
                stopProxyNbiService();
                startBluetoothNbiService();
            } else {
                Toast.makeText(this, R.string.bt_disabled_error, LENGTH_LONG)
                        .show();
                finish();
            }
        } else {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    private void initNbiService() {
        if (HomeKeeperConfig.getMode(this, null) == Mode.DIRECT) {
            if (!BluetoothUtils.enableBluetooth(this)) {
                Toast.makeText(this, R.string.bt_not_supported_error,
                        LENGTH_LONG).show();
                finish();
            }
            if (BluetoothUtils.isEnabled()) {
                stopProxyNbiService();
                startBluetoothNbiService();
            }
        } else if (HomeKeeperConfig.getMode(this, null) == Mode.PROXY) {
            stopBluetoothNbiService();
            startProxyNbiService();
        } else {
            stopBluetoothNbiService();
            stopProxyNbiService();
        }
    }

    private void startBluetoothNbiService() {
        if (activeDev != null
                && !activeDev.equals(HomeKeeperConfig.getBluetoothDevice(this))) {
            stopBluetoothNbiService();
        }
        if (HomeKeeperConfig.getBluetoothDevice(this) != null) {
            startService(new Intent(this, BluetoothNbiService.class));
            activeDev = HomeKeeperConfig.getBluetoothDevice(this);
            modelView
                    .getTitle()
                    .getModel()
                    .setName(
                            HomeKeeperConfig.getBluetoothDevice(this).getName());
        } else {
            modelView.getTitle().getModel().setName(null);
        }
        modelView.getTitle().getModel().setMode(HomeKeeperConfig.getMode(this));
        modelView.getTitle().render();
        modelView.getInfo().render();
    }

    private void startProxyNbiService() {
        if (activeProxy != null
                && !activeProxy.equals(HomeKeeperConfig.getNbiProxy(this))) {
            stopProxyNbiService();
        }
        if (HomeKeeperConfig.getNbiProxy(this) != null) {
            startService(new Intent(this, ProxyNbiService.class));
            activeProxy = HomeKeeperConfig.getNbiProxy(this);
            modelView
                    .getTitle()
                    .getModel()
                    .setName(
                            HomeKeeperConfig.getNbiProxy(this).getHost()
                                    + ":"
                                    + HomeKeeperConfig.getNbiProxy(this)
                                            .getPort());
        } else {
            modelView.getTitle().getModel().setName(null);
        }
        modelView.getTitle().getModel().setMode(HomeKeeperConfig.getMode(this));
        modelView.getTitle().render();
        modelView.getInfo().render();
    }

    private void stopBluetoothNbiService() {
        stopService(new Intent(this, BluetoothNbiService.class));
        activeDev = null;
    }

    private void stopProxyNbiService() {
        stopService(new Intent(this, ProxyNbiService.class));
        activeProxy = null;
    }

    public class NbiServiceDataReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Constants.CONTROLLER_DATA_TRANSFER_ACTION.equals(action)) {
                Parcelable data = intent.getParcelableExtra(Constants.DATA_KEY);
                if (data instanceof CurrentStatusResponse) {
                    CurrentStatusResponse stats = (CurrentStatusResponse) data;

                    boolean gotValidStats = false;

                    // sensors
                    for (int i = 0; i < stats.getSensors().size(); i++) {
                        SensorModel sensor = stats.getSensors().valueAt(i)
                                .getData();
                        if (ViewUtils.validSensor(sensor)) {
                            modelView.getSensor(sensor.getId()).getModel()
                                    .update(sensor);
                            modelView.getSensor(sensor.getId()).render();
                            gotValidStats = true;
                        }
                    }

                    // nodes
                    for (int i = 0; i < stats.getNodes().size(); i++) {
                        NodeModel node = stats.getNodes().valueAt(i).getData();
                        if (ViewUtils.validNode(node)) {
                            modelView.getNode(node.getId()).getModel()
                                    .update(node);
                            modelView.getNode(node.getId()).render();
                            gotValidStats = true;
                        }
                    }

                    if (gotValidStats) {
                        modelView.getInfo().getModel()
                                .setTimestamp(stats.getTimestamp());
                        modelView.getInfo().render();
                        // Toast.makeText(MainActivity.this,
                        // R.string.updating_message, LENGTH_LONG).show();
                    }
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
                                        TopModelView.NODES_NAME_VIEW_MAP
                                                .get(node.getId()))));
                    } else {
                        msg = String.format(getResources().getString(
                                R.string.switch_state_off_message,
                                getResources().getString(
                                        TopModelView.NODES_NAME_VIEW_MAP
                                                .get(node.getId()))));
                    }
                    Toast.makeText(MainActivity.this, msg, LENGTH_LONG).show();
                } else {
                    // unknown data. ignoring
                }
            }
        }
    }

    public class NbiServiceStatusReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Constants.SERVICE_STATUS_ACTION.equals(action)) {
                modelView
                        .getStatus()
                        .getModel()
                        .setState(
                                ServiceState.fromString(intent
                                        .getStringExtra(Constants.SERVICE_STATUS_KEY)));
                modelView
                        .getStatus()
                        .getModel()
                        .setDetails(
                                intent.getStringExtra(Constants.SERVICE_STATUS_DETAILS_KEY));

                modelView.getStatus().render();
            }
        }
    }

    public class BluetoothStateChangedReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)
                    && HomeKeeperConfig.getMode(MainActivity.this, null) == Mode.DIRECT) {
                int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE,
                        BluetoothAdapter.ERROR);
                switch (state) {
                case BluetoothAdapter.STATE_OFF:
                    stopBluetoothNbiService();
                    break;
                case BluetoothAdapter.STATE_ON:
                    stopProxyNbiService();
                    startBluetoothNbiService();
                    break;
                }
            }
        }
    }

    public class NetworkStateChangedReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ConnectivityManager.CONNECTIVITY_ACTION.equals(action)
                    && HomeKeeperConfig.getMode(MainActivity.this, null) == Mode.PROXY) {
                ConnectivityManager cm = (ConnectivityManager) context
                        .getSystemService(Context.CONNECTIVITY_SERVICE);
                NetworkInfo netInfo = cm.getActiveNetworkInfo();
                if (netInfo != null && netInfo.isConnected()) {
                    stopProxyNbiService();
                    startProxyNbiService();
                } else {
                    stopProxyNbiService();
                }
            }
        }
    }
}

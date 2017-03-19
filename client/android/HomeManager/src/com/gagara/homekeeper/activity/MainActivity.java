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
import static com.gagara.homekeeper.common.ControllerConfig.NODE_HEATING_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_SB_HEATER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_TH_ROOM1_PRIMARY_HEATER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_TH_ROOM1_SB_HEATER_ID;
import static com.gagara.homekeeper.ui.viewmodel.TopModelView.NODES;
import static com.gagara.homekeeper.ui.viewmodel.TopModelView.NODES_VIEW_LIST;
import static com.gagara.homekeeper.ui.viewmodel.TopModelView.SENSORS;
import static com.gagara.homekeeper.ui.viewmodel.TopModelView.SENSORS_VIEW_LIST;

import java.util.Date;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Typeface;
import android.net.ConnectivityManager;
import android.os.Bundle;
import android.os.Parcelable;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.app.ActionBarActivity;
import android.util.SparseIntArray;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TableLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.activity.ConfigureNodeDialog.ConfigureNodeListener;
import com.gagara.homekeeper.activity.ManageNodeStateDialog.SwitchNodeStateListener;
import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.Mode;
import com.gagara.homekeeper.common.Proxy;
import com.gagara.homekeeper.model.NodeModel;
import com.gagara.homekeeper.model.ServiceStatusModel;
import com.gagara.homekeeper.model.StateSensorModel;
import com.gagara.homekeeper.model.ValueSensorModel;
import com.gagara.homekeeper.nbi.request.ConfigurationRequest;
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
import com.gagara.homekeeper.ui.viewmodel.StateSensorModelView;
import com.gagara.homekeeper.ui.viewmodel.TopModelView;
import com.gagara.homekeeper.ui.viewmodel.ValueSensorModelView;
import com.gagara.homekeeper.utils.BluetoothUtils;
import com.gagara.homekeeper.utils.HomeKeeperConfig;
import com.gagara.homekeeper.utils.NetworkUtils;

public class MainActivity extends ActionBarActivity implements SwitchNodeStateListener, ConfigureNodeListener {

    private BroadcastReceiver btStateChangedReceiver;
    private BroadcastReceiver proxyStateChangedReceiver;
    private BroadcastReceiver dataReceiver;
    private BroadcastReceiver serviceTitleReceiver;
    private BroadcastReceiver serviceStatusReceiver;

    private TopModelView modelView;

    @SuppressLint("InlinedApi")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ViewGroup contentRoot = (ViewGroup) this.findViewById(R.id.contentRootNode);

        int pos = 0;
        int bgColor1 = getResources().getColor(R.color.row_background_1);
        int bgColor2 = getResources().getColor(R.color.row_background_2);

        // sensors
        for (int i = 0; i < SENSORS_VIEW_LIST.size(); i++) {
            int sId = SENSORS_VIEW_LIST.valueAt(i);
            int sName = SENSORS.get(SENSORS_VIEW_LIST.valueAt(i));
            // outer
            LinearLayout outer = new LinearLayout(this);
            outer.setLayoutParams(new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));
            if (pos % 2 == 0) {
                outer.setBackgroundColor(bgColor1);
            } else {
                outer.setBackgroundColor(bgColor2);
            }
            outer.setGravity(Gravity.CENTER);
            outer.setOrientation(LinearLayout.HORIZONTAL);

            // icon
            ImageView icon = new ImageView(this);
            icon.setLayoutParams(new TableLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
            icon.setContentDescription(getResources().getString(R.string.sensor_icon_descr));
            icon.setImageResource(R.drawable.ic_info_outline_black_24dp);

            // info
            LinearLayout info = new LinearLayout(this);
            info.setLayoutParams(new TableLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT, 0.25f));
            info.setOrientation(LinearLayout.VERTICAL);

            // name
            TextView name = new TextView(this);
            name.setLayoutParams(new TableLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));
            name.setGravity(Gravity.START);
            name.setText(sName);

            // details
            TextView details = new TextView(this);
            details.setId(ViewUtils.getSensorDetailsViewId(sId));
            details.setLayoutParams(new TableLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));
            details.setGravity(Gravity.START);
            details.setTypeface(details.getTypeface(), Typeface.ITALIC);
            details.setTextSize(Double.valueOf(name.getTextSize() * 0.5).floatValue());

            // value
            TextView value = new TextView(this);
            value.setId(ViewUtils.getSensorValueViewId(sId));
            value.setLayoutParams(new TableLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT, 1f));

            info.addView(name);
            info.addView(details);

            outer.addView(icon);
            outer.addView(info);
            outer.addView(value);

            contentRoot.addView(outer, pos);
            pos++;
        }

        // nodes
        for (int i = 0; i < NODES_VIEW_LIST.size(); i++) {
            int nId = NODES_VIEW_LIST.valueAt(i);
            int nName = NODES.get(NODES_VIEW_LIST.valueAt(i));
            // outer
            LinearLayout outer = new LinearLayout(this);
            outer.setLayoutParams(new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));
            if (pos % 2 == 0) {
                outer.setBackgroundColor(bgColor1);
            } else {
                outer.setBackgroundColor(bgColor2);
            }
            outer.setGravity(Gravity.CENTER);
            outer.setOrientation(LinearLayout.HORIZONTAL);

            // icon
            ImageView icon = new ImageView(this);
            icon.setLayoutParams(new TableLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
            icon.setContentDescription(getResources().getString(R.string.node_icon_descr));
            icon.setImageResource(R.drawable.ic_settings_applications_black_24dp);

            // info
            LinearLayout info = new LinearLayout(this);
            info.setId(ViewUtils.getNodeInfoViewId(nId));
            info.setLayoutParams(new TableLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT, 0.4f));
            info.setOrientation(LinearLayout.VERTICAL);
            info.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    onConfigNode(v);
                }
            });

            // infoTop
            LinearLayout infoTop = new LinearLayout(this);
            infoTop.setLayoutParams(new TableLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));
            infoTop.setOrientation(LinearLayout.HORIZONTAL);

            // name
            TextView name = new TextView(this);
            name.setLayoutParams(new TableLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
            name.setGravity(Gravity.START);
            name.setText(nName);

            // config
            TextView config = new TextView(this);
            config.setId(ViewUtils.getNodeConfigViewId(nId));
            config.setLayoutParams(new TableLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
            config.setGravity(Gravity.END);

            // details
            TextView details = new TextView(this);
            details.setId(ViewUtils.getNodeDetailsViewId(nId));
            details.setLayoutParams(new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));
            details.setGravity(Gravity.START);
            details.setTypeface(details.getTypeface(), Typeface.ITALIC);
            details.setTextSize(Double.valueOf(name.getTextSize() * 0.5).floatValue());

            // value
            ToggleButton value = new ToggleButton(this);
            value.setId(ViewUtils.getSensorValueViewId(nId));
            value.setLayoutParams(new TableLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT, 0.85f));
            value.setChecked(false);
            value.setEnabled(false);
            value.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    onManageNode(v);
                }
            });

            infoTop.addView(name);
            infoTop.addView(config);

            info.addView(infoTop);
            info.addView(details);

            outer.addView(icon);
            outer.addView(info);
            outer.addView(value);

            contentRoot.addView(outer, pos);
            pos++;
        }

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
        NodeModelView nodeView = modelView.getNodes().get(ViewUtils.getNodeIdByValueViewId(nodeToggle.getId()));
        if (nodeView != null) {
            ManageNodeStateDialog dialog = new ManageNodeStateDialog();
            dialog.setNodeId(nodeView.getModel().getId());
            dialog.setNodeName(getResources().getString(TopModelView.NODES.get(nodeView.getModel().getId())));
            dialog.setManualMode(!nodeView.getModel().isForcedMode());
            dialog.setState(!nodeView.getModel().getState());
            dialog.setPeriod(nodeView.getModel().getState() ? DEFAULT_SWITCH_OFF_PERIOD_SEC
                    : DEFAULT_SWITCH_ON_PERIOD_SEC);
            dialog.show(getSupportFragmentManager(), Constants.SWITCH_NODE_DIALOG_TAG);
        }
        nodeView.render();
    }

    public void onConfigNode(View view) {
        NodeModelView nodeView = modelView.getNodes().get(ViewUtils.getNodeIdByInfoViewId(view.getId()));
        if (nodeView != null) {
            ConfigureNodeDialog dialog = new ConfigureNodeDialog();
            dialog.setNodeId(nodeView.getModel().getId());
            dialog.setNodeName(getResources().getString(TopModelView.NODES.get(nodeView.getModel().getId())));
            if (nodeView.getModel().getSensorsThresholds().size() > 0) {
                dialog.setSensorsThresholds(nodeView.getModel().getSensorsThresholds());
                dialog.show(getSupportFragmentManager(), Constants.SWITCH_NODE_DIALOG_TAG);
                nodeView.render();
            }
        }
    }

    @Override
    public void doSwitchNodeState(ManageNodeStateDialog dialog) {
        NodeStateChangeRequest request = new NodeStateChangeRequest();

        request.setId(dialog.getNodeId());
        if (dialog.isManualMode()) {
            request.setState(dialog.getState());
            if (dialog.getPeriod() != 0) {
                request.setPeriod(dialog.getPeriod() * 1000L);
            }
        }

        Intent intent = new Intent();
        intent.setAction(CONTROLLER_CONTROL_COMMAND_ACTION);
        intent.putExtra(COMMAND_KEY, request);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    @Override
    public void doNotSwitchNodeState(ManageNodeStateDialog dialog) {
        // do nothing
    }

    @Override
    public void doConfigure(ConfigureNodeDialog dialog) {
        SparseIntArray values = dialog.getResult();
        for (int i = 0; i < values.size(); i++) {
            ConfigurationRequest request = new ConfigurationRequest();

            request.setSensorId(values.keyAt(i));
            request.setSensorThreshold(values.valueAt(i));

            Intent intent = new Intent();
            intent.setAction(CONTROLLER_CONTROL_COMMAND_ACTION);
            intent.putExtra(COMMAND_KEY, request);
            LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
        }
    }

    @Override
    public void doNotConfigure(ConfigureNodeDialog dialog) {
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

                    // value sensor
                    if (stats.getValueSensor() != null) {
                        ValueSensorModel sensor = stats.getValueSensor().getData();
                        if (ViewUtils.validSensor(sensor)) {
                            ((ValueSensorModelView) modelView.getSensor(sensor.getId())).getModel().update(sensor);
                            modelView.getSensor(sensor.getId()).render();
                            latestTimestamp = stats.getTimestamp();
                        }
                    }
                    // state sensor
                    if (stats.getStateSensor() != null) {
                        StateSensorModel sensor = stats.getStateSensor().getData();
                        if (ViewUtils.validSensor(sensor)) {
                            ((StateSensorModelView) modelView.getSensor(sensor.getId())).getModel().update(sensor);
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
                        modelView.getNode(node.getId()).getModel().update(node);
                        modelView.getNode(node.getId()).render();
                        for (int i = 0; i < node.getSensors().size(); i++) {
                            ValueSensorModel sensor = node.getSensors().valueAt(i);
                            if (ViewUtils.validSensor(sensor)) {
                                ((ValueSensorModelView) modelView.getSensor(sensor.getId())).getModel().update(sensor);
                                modelView.getSensor(sensor.getId()).render();
                            }
                        }
                        latestTimestamp = stats.getTimestamp();

                        String msg = null;
                        if (node.getState()) {
                            msg = String.format(getResources().getString(R.string.switch_state_on_message,
                                    getResources().getString(TopModelView.NODES.get(node.getId()))));
                        } else {
                            msg = String.format(getResources().getString(R.string.switch_state_off_message,
                                    getResources().getString(TopModelView.NODES.get(node.getId()))));
                        }
                        Toast.makeText(MainActivity.this, msg, LENGTH_LONG).show();
                    }
                } else if (data instanceof SensorThresholdConfigurationResponse) {
                    SensorThresholdConfigurationResponse cfg = (SensorThresholdConfigurationResponse) data;
                    if (cfg.getData() != null) {
                        ValueSensorModel sensor = cfg.getData();
                        // hardcoded for now
                        if (ViewUtils.validSensor(sensor) && sensor.getId() == SENSOR_TH_ROOM1_SB_HEATER_ID) {
                            int nodeId = NODE_SB_HEATER_ID;
                            modelView.getNode(nodeId).getModel().addSensorThreshold(sensor);
                            modelView.getNode(nodeId).render();
                            latestTimestamp = cfg.getTimestamp();
                        } else if (ViewUtils.validSensor(sensor) && sensor.getId() == SENSOR_TH_ROOM1_PRIMARY_HEATER_ID) {
                            int nodeId = NODE_HEATING_ID;
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

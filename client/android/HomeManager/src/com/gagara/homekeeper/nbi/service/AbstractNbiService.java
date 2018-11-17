package com.gagara.homekeeper.nbi.service;

import static com.gagara.homekeeper.common.Constants.COMMAND_KEY;
import static com.gagara.homekeeper.common.Constants.CONTROLLER_CONTROL_COMMAND_ACTION;
import static com.gagara.homekeeper.common.Constants.SERVICE_STATUS_CHANGE_ACTION;
import static com.gagara.homekeeper.common.Constants.SERVICE_STATUS_DETAILS_KEY;
import static com.gagara.homekeeper.common.Constants.SERVICE_TITLE_CHANGE_ACTION;

import java.io.IOException;
import java.util.Date;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.activity.Main;
import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.nbi.request.Request;
import com.gagara.homekeeper.nbi.response.ConfigurationResponse;
import com.gagara.homekeeper.nbi.response.CurrentStatusResponse;
import com.gagara.homekeeper.nbi.response.NodeStateChangeResponse;
import com.gagara.homekeeper.nbi.response.SensorThresholdConfigurationResponse;
import com.gagara.homekeeper.ui.view.ViewUtils;
import com.gagara.homekeeper.ui.viewmodel.TopModelView;
import com.gagara.homekeeper.utils.NetworkUtils;

public abstract class AbstractNbiService {

    private static final String TAG = AbstractNbiService.class.getName();

    protected static final long INITIAL_CLOCK_SYNC_INTERVAL_SEC = 10;

    protected BroadcastReceiver controllerCommandReceiver = null;

    private BroadcastReceiver networkStateChangedReceiver;

    abstract void send(Request request);

    // abstract String getServiceProviderName();

    public void init() {
        networkStateChangedReceiver = new NetworkStateChangedReceiver();
        Main.getAppContext().registerReceiver(networkStateChangedReceiver,
                new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION));
    }

    public void destroy() {
        Main.getAppContext().unregisterReceiver(networkStateChangedReceiver);
    }

    protected boolean start() {
        if (NetworkUtils.isEnabled(Main.getAppContext())) {
            Log.i(TAG, "starting");
            return true;
        } else {
            Intent intent = new Intent(SERVICE_STATUS_CHANGE_ACTION);
            intent.putExtra(SERVICE_STATUS_DETAILS_KEY,
                    Main.getAppContext().getResources().getString(R.string.networks_disabled_error));
            LocalBroadcastManager.getInstance(Main.getAppContext()).sendBroadcast(intent);
            return false;
        }
    }

    public boolean stop() {
        Log.i(TAG, "stopping");
        // update title
        LocalBroadcastManager.getInstance(Main.getAppContext()).sendBroadcast(new Intent(SERVICE_TITLE_CHANGE_ACTION));
        LocalBroadcastManager.getInstance(Main.getAppContext()).unregisterReceiver(controllerCommandReceiver);
        return true;
    }

    public boolean pause() {
        return false;
    }

    protected void processMessage(JSONObject message, Date timestamp) throws JSONException, IOException {
        ControllerConfig.MessageType msgType = ControllerConfig.MessageType.forCode(message
                .getString(ControllerConfig.MSG_TYPE_KEY));
        Intent intent = new Intent();
        intent.setAction(Constants.CONTROLLER_DATA_TRANSFER_ACTION);
        if (msgType == ControllerConfig.MessageType.CURRENT_STATUS_REPORT) {
            CurrentStatusResponse stats = new CurrentStatusResponse().fromJson(message);
            if (stats != null) {
                intent.putExtra(Constants.DATA_KEY, stats);
                LocalBroadcastManager.getInstance(Main.getAppContext()).sendBroadcast(intent);

                String title = null;
                String name = null;
                if (stats.getNode() != null) {
                    title = Main.getAppContext().getResources().getString(R.string.node_title);
                    if (ViewUtils.validNode(stats.getNode().getData())) {
                        name = Main.getAppContext().getResources()
                                .getString(TopModelView.NODES.get(stats.getNode().getData().getId()));
                    } else {
                        name = stats.getNode().getData().getId() + "";
                    }
                } else if (stats.getValueSensor() != null) {
                    title = Main.getAppContext().getResources().getString(R.string.sensor_title);
                    if (ViewUtils.validSensor(stats.getValueSensor().getData())) {
                        name = Main.getAppContext().getResources()
                                .getString(TopModelView.SENSORS.get(stats.getValueSensor().getData().getId()));
                    } else {
                        name = stats.getValueSensor().getData().getId() + "";
                    }
                } else if (stats.getStateSensor() != null) {
                    title = Main.getAppContext().getResources().getString(R.string.sensor_title);
                    if (ViewUtils.validSensor(stats.getStateSensor().getData())) {
                        name = Main.getAppContext().getResources()
                                .getString(TopModelView.SENSORS.get(stats.getStateSensor().getData().getId()));
                    } else {
                        name = stats.getStateSensor().getData().getId() + "";
                    }
                }

                notifyStatusChange(String
                        .format(Main.getAppContext().getResources().getString(R.string.service_csr_message_status),
                                title, name));
            }
        } else if (msgType == ControllerConfig.MessageType.NODE_STATE_CHANGED) {
            NodeStateChangeResponse nodeState = new NodeStateChangeResponse().fromJson(message);
            if (nodeState != null) {
                intent.putExtra(Constants.DATA_KEY, nodeState);
                LocalBroadcastManager.getInstance(Main.getAppContext()).sendBroadcast(intent);

                String title = null;
                String name = null;
                title = Main.getAppContext().getResources().getString(R.string.node_title);
                if (ViewUtils.validNode(nodeState.getData())) {
                    name = Main.getAppContext().getResources()
                            .getString(TopModelView.NODES.get(nodeState.getData().getId()));
                } else {
                    name = nodeState.getData().getId() + "";
                }

                notifyStatusChange(String
                        .format(Main.getAppContext().getResources().getString(R.string.service_nsc_message_status),
                                title, name));
            }
        } else if (msgType == ControllerConfig.MessageType.CONFIGURATION) {
            ConfigurationResponse conf = new ConfigurationResponse().fromJson(message);
            if (conf != null) {
                if (conf instanceof SensorThresholdConfigurationResponse) {
                    SensorThresholdConfigurationResponse sensorConf = (SensorThresholdConfigurationResponse) conf;
                    intent.putExtra(Constants.DATA_KEY, sensorConf);
                    LocalBroadcastManager.getInstance(Main.getAppContext()).sendBroadcast(intent);

                    String title = null;
                    String name = null;
                    title = Main.getAppContext().getResources().getString(R.string.sensor_title);
                    if (ViewUtils.validSensor(sensorConf.getData())) {
                        name = Main.getAppContext().getResources()
                                .getString(TopModelView.SENSORS_THRESHOLDS.get(sensorConf.getData().getId()));
                    } else {
                        name = sensorConf.getData().getId() + "";
                    }
                    notifyStatusChange(String.format(
                            Main.getAppContext().getResources().getString(R.string.service_cfg_message_status), title,
                            name));
                }
            }
        } else {
            // unknown message
            Log.w(TAG, "ignore message type:" + msgType);
        }
    }

    protected final void notifyStatusChange(CharSequence details) {
        Intent intent = new Intent();
        intent.setAction(SERVICE_STATUS_CHANGE_ACTION);
        if (details != null) {
            intent.putExtra(SERVICE_STATUS_DETAILS_KEY, details);
        }
        LocalBroadcastManager.getInstance(Main.getAppContext()).sendBroadcast(intent);
    }

    public class ControllerCommandsReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (CONTROLLER_CONTROL_COMMAND_ACTION.equals(action)) {
                Request command = intent.getParcelableExtra(COMMAND_KEY);
                send(command);
            }
        }
    }

    private class NetworkStateChangedReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ConnectivityManager.CONNECTIVITY_ACTION.equals(action)) {
                if (NetworkUtils.isEnabled(context)) {
                    start();
                } else {
                    stop();
                }
            }
        }
    }

}

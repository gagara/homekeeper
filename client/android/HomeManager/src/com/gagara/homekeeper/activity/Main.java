package com.gagara.homekeeper.activity;

import static android.widget.Toast.LENGTH_LONG;
import static com.gagara.homekeeper.common.Constants.CFG_SELECTED_GATEWAY_ID;
import static com.gagara.homekeeper.common.Constants.COMMAND_KEY;
import static com.gagara.homekeeper.common.Constants.CONTROLLER_CONTROL_COMMAND_ACTION;
import static com.gagara.homekeeper.common.Constants.DEFAULT_SWITCH_OFF_PERIOD_SEC;
import static com.gagara.homekeeper.common.Constants.DEFAULT_SWITCH_ON_PERIOD_SEC;
import static com.gagara.homekeeper.common.Constants.SERVICE_STATUS_CHANGE_ACTION;
import static com.gagara.homekeeper.common.Constants.SERVICE_STATUS_DETAILS_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_HEATING_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_SB_HEATER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_TH_ROOM1_PRIMARY_HEATER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_TH_ROOM1_SB_HEATER_ID;
import static com.gagara.homekeeper.ui.viewmodel.TopModelView.NODES;
import static com.gagara.homekeeper.ui.viewmodel.TopModelView.NODES_VIEW_LIST;
import static com.gagara.homekeeper.ui.viewmodel.TopModelView.SENSORS;
import static com.gagara.homekeeper.ui.viewmodel.TopModelView.SENSORS_VIEW_LIST;

import java.util.Date;
import java.util.Map;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences.Editor;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Parcelable;
import android.preference.PreferenceManager;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.app.ActionBarActivity;
import android.util.SparseIntArray;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AnimationUtils;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TableLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.activity.ConfigureNodeDialog.ConfigureNodeListener;
import com.gagara.homekeeper.activity.ManageNodeStateDialog.SwitchNodeStateListener;
import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.Gateway;
import com.gagara.homekeeper.model.NodeModel;
import com.gagara.homekeeper.model.ServiceStatusModel;
import com.gagara.homekeeper.model.StateSensorModel;
import com.gagara.homekeeper.model.ValueSensorModel;
import com.gagara.homekeeper.nbi.request.ConfigurationRequest;
import com.gagara.homekeeper.nbi.request.NodeStateChangeRequest;
import com.gagara.homekeeper.nbi.response.CurrentStatusResponse;
import com.gagara.homekeeper.nbi.response.NodeStateChangeResponse;
import com.gagara.homekeeper.nbi.response.SensorThresholdConfigurationResponse;
import com.gagara.homekeeper.nbi.service.GatewayNbiService;
import com.gagara.homekeeper.ui.view.ViewUtils;
import com.gagara.homekeeper.ui.viewmodel.NodeModelView;
import com.gagara.homekeeper.ui.viewmodel.StateSensorModelView;
import com.gagara.homekeeper.ui.viewmodel.TopModelView;
import com.gagara.homekeeper.ui.viewmodel.ValueSensorModelView;
import com.gagara.homekeeper.utils.HomeKeeperConfig;

public class Main extends ActionBarActivity implements SwitchNodeStateListener, ConfigureNodeListener {

    private static Context context;

    private BroadcastReceiver dataReceiver;
    private BroadcastReceiver serviceStatusReceiver;

    private GatewayNbiService gatewayService;
    private NbiGatewayChangeListener gatewaySelectListener;

    private TopModelView modelView;

    public static Context getAppContext() {
        return context;
    }

    @SuppressLint("InlinedApi")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        context = getApplicationContext();
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

        dataReceiver = new NbiServiceDataReceiver();
        serviceStatusReceiver = new NbiServiceStatusReceiver();

        gatewaySelectListener = new NbiGatewayChangeListener();
        ((Spinner) modelView.getTitle().getView()).setOnItemSelectedListener(gatewaySelectListener);

        gatewayService = new GatewayNbiService();
        gatewayService.init();
    }

    @Override
    protected void onResume() {
        super.onResume();
        LocalBroadcastManager.getInstance(this).registerReceiver(dataReceiver,
                new IntentFilter(Constants.CONTROLLER_DATA_TRANSFER_ACTION));
        LocalBroadcastManager.getInstance(this).registerReceiver(serviceStatusReceiver,
                new IntentFilter(SERVICE_STATUS_CHANGE_ACTION));
        modelView.getTitle().render();
        gatewayService.start();
    }

    @Override
    protected void onPause() {
        super.onPause();
        LocalBroadcastManager.getInstance(this).unregisterReceiver(dataReceiver);
        LocalBroadcastManager.getInstance(this).unregisterReceiver(serviceStatusReceiver);
        gatewayService.pause();
    }

    @Override
    protected void onStop() {
        super.onStop();
        gatewayService.stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        gatewayService.destroy();
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
            Intent intent = new Intent(this, Settings.class);
            startActivity(intent);
            return true;
        } else if (id == R.id.action_refresh) {
            gatewayService.refresh();
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
                dialog.show(getSupportFragmentManager(), Constants.CONFIG_NODE_DIALOG_TAG);
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
                request.setPeriod((long) dialog.getPeriod());
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

    public class NbiGatewayChangeListener implements OnItemSelectedListener {

        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
            Map<String, Gateway> gateways = HomeKeeperConfig.getAllNbiGateways(Main.getAppContext());
            String[] ids = gateways.keySet().toArray(new String[] {});
            if (position < ids.length) {
                String prevId = PreferenceManager.getDefaultSharedPreferences(context).getString(
                        CFG_SELECTED_GATEWAY_ID, "");
                if (!prevId.equals(ids[position])) {
                    Editor editor = PreferenceManager.getDefaultSharedPreferences(context).edit();
                    editor.putString(CFG_SELECTED_GATEWAY_ID, ids[position]);
                    editor.commit();
                    gatewayService.stop();
                    gatewayService.start();
                }
            }
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {
        }
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
                        Toast.makeText(Main.this, msg, LENGTH_LONG).show();
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

    // public class NbiServiceTitleReceiver extends BroadcastReceiver {
    // public void onReceive(Context context, Intent intent) {
    // String action = intent.getAction();
    // if (SERVICE_TITLE_CHANGE_ACTION.equals(action)) {
    // modelView.getTitle().getModel().setId(intent.getStringExtra(GATEWAY_ID_KEY));
    // modelView.getTitle().render();
    // }
    // }
    // }

    public class NbiServiceStatusReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (SERVICE_STATUS_CHANGE_ACTION.equals(action)) {
                ServiceStatusModel status = new ServiceStatusModel();
                status.setDetails(intent.getStringExtra(SERVICE_STATUS_DETAILS_KEY));
                status.setTimestamp(new Date());
                modelView.getLog().getModel().add(status);
                modelView.getLog().getView()
                        .setAnimation(AnimationUtils.loadAnimation(context, R.anim.abc_slide_in_bottom));

                modelView.getLog().render();
            }
        }
    }
}

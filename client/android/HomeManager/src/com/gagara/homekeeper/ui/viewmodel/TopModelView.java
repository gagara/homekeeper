package com.gagara.homekeeper.ui.viewmodel;

import static com.gagara.homekeeper.common.ControllerConfig.NODE_CIRCULATION_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_FLOOR_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_HEATING_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_HOTWATER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_SB_HEATER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_SOLAR_PRIMARY_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_SOLAR_SECONDARY_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_SUPPLY_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_BOILER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_BOILER_POWER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_MIX_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_REVERSE_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_ROOM1_HUM_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_ROOM1_TEMP_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_SB_HEATER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_SOLAR_PRIMAY_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_SOLAR_SECONDARY_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_SUPPLY_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_TANK_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_TH_ROOM1_PRIMARY_HEATER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_TH_ROOM1_SB_HEATER_ID;
import android.app.Activity;
import android.os.Bundle;
import android.util.SparseArray;
import android.util.SparseIntArray;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.Model;
import com.gagara.homekeeper.model.ValueSensorModel;
import com.gagara.homekeeper.model.ValueSensorModel.SensorType;
import com.gagara.homekeeper.ui.view.ViewUtils;

public class TopModelView implements ModelView<Model> {

    public static final SparseIntArray SENSORS;
    public static final SparseIntArray SENSORS_THRESHOLDS;
    public static final SparseIntArray NODES;

    public static final SparseIntArray SENSORS_VIEW_LIST;
    public static final SparseIntArray NODES_VIEW_LIST;

    private ServiceTitleModelView title;
    private ServiceInfoModelView info;
    private ServiceLogModelView log;

    private SparseArray<SensorModelView<?>> sensors;
    private SparseArray<NodeModelView> nodes;

    static {
        SENSORS = new SparseIntArray();
        SENSORS.put(SENSOR_SUPPLY_ID, R.string.sensor_supply_name);
        SENSORS.put(SENSOR_REVERSE_ID, R.string.sensor_reverse_name);
        SENSORS.put(SENSOR_TANK_ID, R.string.sensor_tank_name);
        SENSORS.put(SENSOR_MIX_ID, R.string.sensor_mix_name);
        SENSORS.put(SENSOR_SB_HEATER_ID, R.string.sensor_sb_heater_name);
        SENSORS.put(SENSOR_BOILER_ID, R.string.sensor_boiler_name);
        SENSORS.put(SENSOR_ROOM1_TEMP_ID, R.string.sensor_room1_temp_name);
        SENSORS.put(SENSOR_ROOM1_HUM_ID, R.string.sensor_room1_hum_name);
        SENSORS.put(SENSOR_BOILER_POWER_ID, R.string.sensor_boiler_power_name);
        SENSORS.put(SENSOR_SOLAR_PRIMAY_ID, R.string.sensor_solar_primary_name);
        SENSORS.put(SENSOR_SOLAR_SECONDARY_ID, R.string.sensor_solar_secondary_name);

        SENSORS_THRESHOLDS = new SparseIntArray();
        SENSORS_THRESHOLDS.put(SENSOR_TH_ROOM1_SB_HEATER_ID, R.string.sensor_th_room1_sb_heater);
        SENSORS_THRESHOLDS.put(SENSOR_TH_ROOM1_PRIMARY_HEATER_ID, R.string.sensor_th_room1_primary_heater);

        NODES = new SparseIntArray();
        NODES.put(NODE_SUPPLY_ID, R.string.node_supply_name);
        NODES.put(NODE_HEATING_ID, R.string.node_heating_name);
        NODES.put(NODE_FLOOR_ID, R.string.node_floor_name);
        NODES.put(NODE_SB_HEATER_ID, R.string.node_sb_heater_name);
        NODES.put(NODE_HOTWATER_ID, R.string.node_hotwater_name);
        NODES.put(NODE_CIRCULATION_ID, R.string.node_circulation_name);
        NODES.put(NODE_SOLAR_PRIMARY_ID, R.string.node_solar_primary_name);
        NODES.put(NODE_SOLAR_SECONDARY_ID, R.string.node_solar_secondary_name);

        int idx = 0;
        SENSORS_VIEW_LIST = new SparseIntArray();
        SENSORS_VIEW_LIST.put(idx++, SENSOR_SUPPLY_ID);
        SENSORS_VIEW_LIST.put(idx++, SENSOR_REVERSE_ID);
        SENSORS_VIEW_LIST.put(idx++, SENSOR_TANK_ID);
        SENSORS_VIEW_LIST.put(idx++, SENSOR_MIX_ID);
        SENSORS_VIEW_LIST.put(idx++, SENSOR_SB_HEATER_ID);
        SENSORS_VIEW_LIST.put(idx++, SENSOR_BOILER_ID);
        SENSORS_VIEW_LIST.put(idx++, SENSOR_ROOM1_TEMP_ID);
        SENSORS_VIEW_LIST.put(idx++, SENSOR_ROOM1_HUM_ID);
        SENSORS_VIEW_LIST.put(idx++, SENSOR_BOILER_POWER_ID);
        SENSORS_VIEW_LIST.put(idx++, SENSOR_SOLAR_PRIMAY_ID);
        SENSORS_VIEW_LIST.put(idx++, SENSOR_SOLAR_SECONDARY_ID);
        idx = 0;
        NODES_VIEW_LIST = new SparseIntArray();
        NODES_VIEW_LIST.put(idx++, NODE_SUPPLY_ID);
        NODES_VIEW_LIST.put(idx++, NODE_HEATING_ID);
        NODES_VIEW_LIST.put(idx++, NODE_FLOOR_ID);
        NODES_VIEW_LIST.put(idx++, NODE_SB_HEATER_ID);
        NODES_VIEW_LIST.put(idx++, NODE_HOTWATER_ID);
        NODES_VIEW_LIST.put(idx++, NODE_CIRCULATION_ID);
        NODES_VIEW_LIST.put(idx++, NODE_SOLAR_PRIMARY_ID);
        NODES_VIEW_LIST.put(idx++, NODE_SOLAR_SECONDARY_ID);
    }

    public TopModelView(Activity ctx) {
        title = new ServiceTitleModelView((TextView) ctx.findViewById(R.id.headerLeft), ctx.getResources());
        info = new ServiceInfoModelView((TextView) ctx.findViewById(R.id.headerRight), ctx.getResources());
        log = new ServiceLogModelView((TextView) ctx.findViewById(R.id.footerLeft), ctx.getResources());

        sensors = new SparseArray<SensorModelView<?>>();
        for (int i = 0; i < SENSORS.size(); i++) {
            int id = SENSORS.keyAt(i);
            SensorModelView<?> s = null;
            if (ViewUtils.validValueSensorId(id)) {
                s = new ValueSensorModelView(id);
            } else if (ViewUtils.validStateSensorId(id)) {
                s = new StateSensorModelView(id);
            }
            if (s != null) {
                s.setValueView((TextView) ctx.findViewById(ViewUtils.getSensorValueViewId(id)));
                s.setDetailsView((TextView) ctx.findViewById(ViewUtils.getSensorDetailsViewId(id)));
                s.setResources(ctx.getResources());
                if (SENSOR_ROOM1_HUM_ID == id) {
                    ((ValueSensorModel) s.getModel()).setType(SensorType.HUMIDITY);
                }
                sensors.put(id, s);
            }
        }

        nodes = new SparseArray<NodeModelView>();
        for (int i = 0; i < NODES.size(); i++) {
            int id = NODES.keyAt(i);
            NodeModelView n = new NodeModelView(id);
            n.setValueView((ToggleButton) ctx.findViewById(ViewUtils.getNodeValueViewId(id)));
            n.setDetailsView((TextView) ctx.findViewById(ViewUtils.getNodeDetailsViewId(id)));
            n.setConfigView((TextView) ctx.findViewById(ViewUtils.getNodeConfigViewId(id)));
            n.setResources(ctx.getResources());
            nodes.put(id, n);
        }
    }

    @Override
    public void saveState(Bundle bundle) {
        saveState(bundle, "");
    }

    @Override
    public void saveState(Bundle bundle, String prefix) {
        title.saveState(bundle, prefix);
        info.saveState(bundle, prefix);
        log.saveState(bundle, prefix);
        for (int i = 0; i < sensors.size(); i++) {
            sensors.valueAt(i).saveState(bundle, prefix);
        }
        for (int i = 0; i < nodes.size(); i++) {
            nodes.valueAt(i).saveState(bundle, prefix);
        }
    }

    @Override
    public void restoreState(Bundle bundle) {
        restoreState(bundle, "");
    }

    @Override
    public void restoreState(Bundle bundle, String prefix) {
        title.restoreState(bundle, prefix);
        info.restoreState(bundle, prefix);
        log.restoreState(bundle, prefix);
        for (int i = 0; i < sensors.size(); i++) {
            sensors.valueAt(i).restoreState(bundle, prefix);
        }
        for (int i = 0; i < nodes.size(); i++) {
            nodes.valueAt(i).restoreState(bundle, prefix);
        }
    }

    @Override
    public void render() {
        title.render();
        info.render();
        log.render();
        for (int i = 0; i < sensors.size(); i++) {
            sensors.valueAt(i).render();
        }
        for (int i = 0; i < nodes.size(); i++) {
            nodes.valueAt(i).render();
        }
    }

    @Override
    public boolean isInitialized() {
        return true;
    }

    public SparseArray<NodeModelView> getNodes() {
        return nodes;
    }

    public ServiceTitleModelView getTitle() {
        return title;
    }

    public ServiceInfoModelView getInfo() {
        return info;
    }

    public ServiceLogModelView getLog() {
        return log;
    }

    public SensorModelView<?> getSensor(int id) {
        return sensors.get(id);
    }

    public NodeModelView getNode(int id) {
        return nodes.get(id);
    }

    public ValueSensorModelView getSensorSupply() {
        return (ValueSensorModelView) sensors.get(SENSOR_SUPPLY_ID);
    }

    public ValueSensorModelView getSensorReverse() {
        return (ValueSensorModelView) sensors.get(SENSOR_REVERSE_ID);
    }

    public ValueSensorModelView getSensorTank() {
        return (ValueSensorModelView) sensors.get(SENSOR_TANK_ID);
    }

    public ValueSensorModelView getSensorBoiler() {
        return (ValueSensorModelView) sensors.get(SENSOR_BOILER_ID);
    }

    public ValueSensorModelView getSensorMix() {
        return (ValueSensorModelView) sensors.get(SENSOR_MIX_ID);
    }

    public ValueSensorModelView getSensorSbHeater() {
        return (ValueSensorModelView) sensors.get(SENSOR_SB_HEATER_ID);
    }

    public StateSensorModelView getSensorBoilerPower() {
        return (StateSensorModelView) sensors.get(SENSOR_BOILER_POWER_ID);
    }

    public NodeModelView getNodeHeaterToTank() {
        return nodes.get(NODE_SUPPLY_ID);
    }

    public NodeModelView getNodeTankToSystem() {
        return nodes.get(NODE_HEATING_ID);
    }

    public NodeModelView getNodeTankToBoiler() {
        return nodes.get(NODE_HOTWATER_ID);
    }

    public NodeModelView getNodeCirculation() {
        return nodes.get(NODE_CIRCULATION_ID);
    }

    public NodeModelView getNodeSbHeater() {
        return nodes.get(NODE_SB_HEATER_ID);
    }
}

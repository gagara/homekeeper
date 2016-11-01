package com.gagara.homekeeper.ui.viewmodel;

import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_ROOM1_HUM_ID;

import java.util.HashMap;
import java.util.Map;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.model.Model;
import com.gagara.homekeeper.model.SensorModel.SensorType;

@SuppressLint("UseSparseArrays")
public class TopModelView implements ModelView<Model> {

    public static final Map<Integer, Integer> SENSORS_NAME_VIEW_MAP;
    public static final Map<Integer, Integer> NODES_NAME_VIEW_MAP;

    public static final Map<Integer, Integer> SENSORS_VALUE_VIEW_MAP;
    public static final Map<Integer, Integer> SENSORS_DETAILS_VIEW_MAP;

    public static final Map<Integer, Integer> NODES_VALUE_VIEW_MAP;
    public static final Map<Integer, Integer> NODES_DETAILS_VIEW_MAP;
    public static final Map<Integer, Integer> NODES_CONFIG_VIEW_MAP;

    private ServiceTitleModelView title;
    private ServiceInfoModelView info;
    private ServiceLogModelView log;

    private Map<Integer, SensorModelView> sensors;
    private Map<Integer, NodeModelView> nodes;

    static {
        SENSORS_NAME_VIEW_MAP = new HashMap<Integer, Integer>();
        SENSORS_NAME_VIEW_MAP.put(ControllerConfig.SENSOR_SUPPLY_ID, R.string.sensor_supply_name);
        SENSORS_NAME_VIEW_MAP.put(ControllerConfig.SENSOR_REVERSE_ID, R.string.sensor_reverse_name);
        SENSORS_NAME_VIEW_MAP.put(ControllerConfig.SENSOR_TANK_ID, R.string.sensor_tank_name);
        SENSORS_NAME_VIEW_MAP.put(ControllerConfig.SENSOR_BOILER_ID, R.string.sensor_boiler_name);
        SENSORS_NAME_VIEW_MAP.put(ControllerConfig.SENSOR_MIX_ID, R.string.sensor_mix_name);
        SENSORS_NAME_VIEW_MAP.put(ControllerConfig.SENSOR_SB_HEATER_ID, R.string.sensor_sb_heater_name);
        SENSORS_NAME_VIEW_MAP.put(ControllerConfig.SENSOR_ROOM1_TEMP_ID, R.string.sensor_room1_temp_name);
        SENSORS_NAME_VIEW_MAP.put(ControllerConfig.SENSOR_ROOM1_HUM_ID, R.string.sensor_room1_hum_name);

        SENSORS_VALUE_VIEW_MAP = new HashMap<Integer, Integer>();
        SENSORS_VALUE_VIEW_MAP.put(ControllerConfig.SENSOR_SUPPLY_ID, R.id.sensorSupplyValue);
        SENSORS_VALUE_VIEW_MAP.put(ControllerConfig.SENSOR_REVERSE_ID, R.id.sensorReverseValue);
        SENSORS_VALUE_VIEW_MAP.put(ControllerConfig.SENSOR_TANK_ID, R.id.sensorTankValue);
        SENSORS_VALUE_VIEW_MAP.put(ControllerConfig.SENSOR_BOILER_ID, R.id.sensorBoilerValue);
        SENSORS_VALUE_VIEW_MAP.put(ControllerConfig.SENSOR_MIX_ID, R.id.sensorMixValue);
        SENSORS_VALUE_VIEW_MAP.put(ControllerConfig.SENSOR_SB_HEATER_ID, R.id.sensorSbHeaterValue);
        SENSORS_VALUE_VIEW_MAP.put(ControllerConfig.SENSOR_ROOM1_TEMP_ID, R.id.sensorRoom1TempValue);
        SENSORS_VALUE_VIEW_MAP.put(ControllerConfig.SENSOR_ROOM1_HUM_ID, R.id.sensorRoom1HumValue);

        SENSORS_DETAILS_VIEW_MAP = new HashMap<Integer, Integer>();
        SENSORS_DETAILS_VIEW_MAP.put(ControllerConfig.SENSOR_SUPPLY_ID, R.id.sensorSupplyDetails);
        SENSORS_DETAILS_VIEW_MAP.put(ControllerConfig.SENSOR_REVERSE_ID, R.id.sensorReverseDetails);
        SENSORS_DETAILS_VIEW_MAP.put(ControllerConfig.SENSOR_TANK_ID, R.id.sensorTankDetails);
        SENSORS_DETAILS_VIEW_MAP.put(ControllerConfig.SENSOR_BOILER_ID, R.id.sensorBoilerDetails);
        SENSORS_DETAILS_VIEW_MAP.put(ControllerConfig.SENSOR_MIX_ID, R.id.sensorMixDetails);
        SENSORS_DETAILS_VIEW_MAP.put(ControllerConfig.SENSOR_SB_HEATER_ID, R.id.sensorSbHeaterDetails);
        SENSORS_DETAILS_VIEW_MAP.put(ControllerConfig.SENSOR_ROOM1_TEMP_ID, R.id.sensorRoom1TempDetails);
        SENSORS_DETAILS_VIEW_MAP.put(ControllerConfig.SENSOR_ROOM1_HUM_ID, R.id.sensorRoom1HumDetails);

        NODES_NAME_VIEW_MAP = new HashMap<Integer, Integer>();
        NODES_NAME_VIEW_MAP.put(ControllerConfig.NODE_SUPPLY_ID, R.string.node_supply_name);
        NODES_NAME_VIEW_MAP.put(ControllerConfig.NODE_HEATING_ID, R.string.node_heating_name);
        NODES_NAME_VIEW_MAP.put(ControllerConfig.NODE_FLOOR_ID, R.string.node_floor_name);
        NODES_NAME_VIEW_MAP.put(ControllerConfig.NODE_HOTWATER_ID, R.string.node_hotwater_name);
        NODES_NAME_VIEW_MAP.put(ControllerConfig.NODE_CIRCULATION_ID, R.string.node_circulation_name);
        NODES_NAME_VIEW_MAP.put(ControllerConfig.NODE_BOILER_ID, R.string.node_boilder_name);
        NODES_NAME_VIEW_MAP.put(ControllerConfig.NODE_SB_HEATER_ID, R.string.node_sb_heater_name);

        NODES_VALUE_VIEW_MAP = new HashMap<Integer, Integer>();
        NODES_VALUE_VIEW_MAP.put(ControllerConfig.NODE_SUPPLY_ID, R.id.nodeSupplyValue);
        NODES_VALUE_VIEW_MAP.put(ControllerConfig.NODE_HEATING_ID, R.id.nodeHeatingValue);
        NODES_VALUE_VIEW_MAP.put(ControllerConfig.NODE_FLOOR_ID, R.id.nodeFloorValue);
        NODES_VALUE_VIEW_MAP.put(ControllerConfig.NODE_HOTWATER_ID, R.id.nodeHotwaterValue);
        NODES_VALUE_VIEW_MAP.put(ControllerConfig.NODE_CIRCULATION_ID, R.id.nodeCirculationValue);
        NODES_VALUE_VIEW_MAP.put(ControllerConfig.NODE_BOILER_ID, R.id.nodeBoilerValue);
        NODES_VALUE_VIEW_MAP.put(ControllerConfig.NODE_SB_HEATER_ID, R.id.nodeSbHeaterValue);

        NODES_DETAILS_VIEW_MAP = new HashMap<Integer, Integer>();
        NODES_DETAILS_VIEW_MAP.put(ControllerConfig.NODE_SUPPLY_ID, R.id.nodeSupplyDetails);
        NODES_DETAILS_VIEW_MAP.put(ControllerConfig.NODE_HEATING_ID, R.id.nodeHeatingDetails);
        NODES_DETAILS_VIEW_MAP.put(ControllerConfig.NODE_FLOOR_ID, R.id.nodeFloorDetails);
        NODES_DETAILS_VIEW_MAP.put(ControllerConfig.NODE_HOTWATER_ID, R.id.nodeHotwaterDetails);
        NODES_DETAILS_VIEW_MAP.put(ControllerConfig.NODE_CIRCULATION_ID, R.id.nodeCirculationDetails);
        NODES_DETAILS_VIEW_MAP.put(ControllerConfig.NODE_BOILER_ID, R.id.nodeBoilerDetails);
        NODES_DETAILS_VIEW_MAP.put(ControllerConfig.NODE_SB_HEATER_ID, R.id.nodeSbHeaterDetails);

        NODES_CONFIG_VIEW_MAP = new HashMap<Integer, Integer>();
        NODES_CONFIG_VIEW_MAP.put(ControllerConfig.NODE_SUPPLY_ID, R.id.nodeSupplyConfig);
        NODES_CONFIG_VIEW_MAP.put(ControllerConfig.NODE_HEATING_ID, R.id.nodeHeatingConfig);
        NODES_CONFIG_VIEW_MAP.put(ControllerConfig.NODE_FLOOR_ID, R.id.nodeFloorConfig);
        NODES_CONFIG_VIEW_MAP.put(ControllerConfig.NODE_HOTWATER_ID, R.id.nodeHotwaterConfig);
        NODES_CONFIG_VIEW_MAP.put(ControllerConfig.NODE_CIRCULATION_ID, R.id.nodeCirculationConfig);
        NODES_CONFIG_VIEW_MAP.put(ControllerConfig.NODE_BOILER_ID, R.id.nodeBoilerConfig);
        NODES_CONFIG_VIEW_MAP.put(ControllerConfig.NODE_SB_HEATER_ID, R.id.nodeSbHeaterConfig);

    }

    public TopModelView(Activity ctx) {
        title = new ServiceTitleModelView((TextView) ctx.findViewById(R.id.headerLeft), ctx.getResources());
        info = new ServiceInfoModelView((TextView) ctx.findViewById(R.id.headerRight), ctx.getResources());
        log = new ServiceLogModelView((TextView) ctx.findViewById(R.id.footerLeft), ctx.getResources());

        sensors = new HashMap<Integer, SensorModelView>();
        for (Integer id : SENSORS_VALUE_VIEW_MAP.keySet()) {
            SensorModelView s = new SensorModelView(id);
            s.setValueView((TextView) ctx.findViewById(SENSORS_VALUE_VIEW_MAP.get(id)));
            s.setDetailsView((TextView) ctx.findViewById(SENSORS_DETAILS_VIEW_MAP.get(id)));
            s.setResources(ctx.getResources());
            if (SENSOR_ROOM1_HUM_ID == id) {
                s.getModel().setType(SensorType.HUMIDITY);
            }
            sensors.put(id, s);
        }

        nodes = new HashMap<Integer, NodeModelView>();
        for (Integer id : NODES_VALUE_VIEW_MAP.keySet()) {
            NodeModelView n = new NodeModelView(id);
            n.setValueView((ToggleButton) ctx.findViewById(NODES_VALUE_VIEW_MAP.get(id)));
            n.setDetailsView((TextView) ctx.findViewById(NODES_DETAILS_VIEW_MAP.get(id)));
            n.setConfigView((TextView) ctx.findViewById(NODES_CONFIG_VIEW_MAP.get(id)));
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
        for (SensorModelView s : sensors.values()) {
            s.saveState(bundle, prefix);
        }
        for (NodeModelView n : nodes.values()) {
            n.saveState(bundle, prefix);
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
        for (SensorModelView s : sensors.values()) {
            s.restoreState(bundle, prefix);
        }
        for (NodeModelView n : nodes.values()) {
            n.restoreState(bundle, prefix);
        }
    }

    @Override
    public void render() {
        title.render();
        info.render();
        log.render();
        for (SensorModelView s : sensors.values()) {
            s.render();
        }
        for (NodeModelView n : nodes.values()) {
            n.render();
        }
    }

    @Override
    public boolean isInitialized() {
        return true;
    }

    public Map<Integer, SensorModelView> getSensors() {
        return sensors;
    }

    public Map<Integer, NodeModelView> getNodes() {
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

    public SensorModelView getSensor(int id) {
        return sensors.get(id);
    }

    public NodeModelView getNode(int id) {
        return nodes.get(id);
    }

    public SensorModelView getSensorSupply() {
        return sensors.get(ControllerConfig.SENSOR_SUPPLY_ID);
    }

    public SensorModelView getSensorReverse() {
        return sensors.get(ControllerConfig.SENSOR_REVERSE_ID);
    }

    public SensorModelView getSensorTank() {
        return sensors.get(ControllerConfig.SENSOR_TANK_ID);
    }

    public SensorModelView getSensorBoiler() {
        return sensors.get(ControllerConfig.SENSOR_BOILER_ID);
    }

    public SensorModelView getSensorMix() {
        return sensors.get(ControllerConfig.SENSOR_MIX_ID);
    }

    public SensorModelView getSensorSbHeater() {
        return sensors.get(ControllerConfig.SENSOR_SB_HEATER_ID);
    }

    public NodeModelView getNodeHeaterToTank() {
        return nodes.get(ControllerConfig.NODE_SUPPLY_ID);
    }

    public NodeModelView getNodeTankToSystem() {
        return nodes.get(ControllerConfig.NODE_HEATING_ID);
    }

    public NodeModelView getNodeTankToBoiler() {
        return nodes.get(ControllerConfig.NODE_HOTWATER_ID);
    }

    public NodeModelView getNodeCirculation() {
        return nodes.get(ControllerConfig.NODE_CIRCULATION_ID);
    }

    public NodeModelView getNodeBoiler() {
        return nodes.get(ControllerConfig.NODE_BOILER_ID);
    }

    public NodeModelView getNodeSbHeater() {
        return nodes.get(ControllerConfig.NODE_SB_HEATER_ID);
    }
}

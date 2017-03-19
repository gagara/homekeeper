package com.gagara.homekeeper.model;

import static com.gagara.homekeeper.common.Constants.UNDEFINED_DATE;

import java.util.Date;

import android.os.Bundle;
import android.util.SparseArray;

public class NodeModel implements Model {

    private int id;
    private Boolean state = null;
    private Date switchTimestamp = UNDEFINED_DATE;
    private Boolean forcedMode = null;
    private Date forcedModeTimestamp = UNDEFINED_DATE;
    private SparseArray<ValueSensorModel> sensors = new SparseArray<ValueSensorModel>();
    private SparseArray<ValueSensorModel> sensorsThresholds = new SparseArray<ValueSensorModel>();

    public NodeModel(int id) {
        this.id = id;
    }

    @Override
    public void saveState(Bundle bundle) {
        saveState(bundle, "");
    }

    @Override
    public void saveState(Bundle bundle, String prefix) {
        if (state != null) {
            bundle.putBoolean(prefix + "node_" + id + "_state", state);
        }
        bundle.putLong(prefix + "node_" + id + "_switchTs", switchTimestamp.getTime());
        if (forcedMode != null) {
            bundle.putBoolean(prefix + "node_" + id + "_forcedMode", forcedMode);
        }
        bundle.putLong(prefix + "node_" + id + "_forcedModeTs", forcedModeTimestamp.getTime());
        if (sensors.size() > 0) {
            int[] ids = new int[sensors.size()];
            for (int i = 0; i < sensors.size(); i++) {
                sensors.valueAt(i).saveState(bundle, prefix + "node_" + id + "_sensors_");
                ids[i] = sensors.keyAt(i);
            }
            bundle.putIntArray(prefix + "node_" + id + "_sensors_ids", ids);
        }
        if (sensorsThresholds.size() > 0) {
            int[] ids = new int[sensorsThresholds.size()];
            for (int i = 0; i < sensorsThresholds.size(); i++) {
                sensorsThresholds.valueAt(i).saveState(bundle, prefix + "node_" + id + "_sensors_thresholds_");
                ids[i] = sensorsThresholds.keyAt(i);
            }
            bundle.putIntArray(prefix + "node_" + id + "_sensors_thresholds_ids", ids);
        }
    }

    @Override
    public void restoreState(Bundle bundle) {
        restoreState(bundle, "");
    }

    @Override
    public void restoreState(Bundle bundle, String prefix) {
        if (bundle.containsKey(prefix + "node_" + id + "_state")) {
            state = bundle.getBoolean(prefix + "node_" + id + "_state");
        }
        switchTimestamp = new Date(bundle.getLong(prefix + "node_" + id + "_switchTs"));
        if (bundle.containsKey(prefix + "node_" + id + "_forcedMode")) {
            forcedMode = bundle.getBoolean(prefix + "node_" + id + "_forcedMode");
        }
        forcedModeTimestamp = new Date(bundle.getLong(prefix + "node_" + id + "_forcedModeTs"));
        int[] ids = bundle.getIntArray(prefix + "node_" + id + "_sensors_ids");
        if (ids != null) {
            sensors.clear();
            for (int i = 0; i < ids.length; i++) {
                ValueSensorModel sm = new ValueSensorModel(ids[i]);
                sm.restoreState(bundle, prefix + "node_" + id + "_sensors_");
                sensors.put(ids[i], sm);
            }
        }
        int[] thIds = bundle.getIntArray(prefix + "node_" + id + "_sensors_thresholds_ids");
        if (thIds != null) {
            sensorsThresholds.clear();
            for (int i = 0; i < thIds.length; i++) {
                ValueSensorModel sm = new ValueSensorModel(thIds[i]);
                sm.restoreState(bundle, prefix + "node_" + id + "_sensors_thresholds_");
                sensorsThresholds.put(thIds[i], sm);
            }
        }
    }

    public void update(NodeModel newModel) {
        if (newModel.getState() != null) {
            state = newModel.getState();
        }
        if (newModel.getSwitchTimestamp() != UNDEFINED_DATE && !switchTimestamp.equals(newModel.getSwitchTimestamp())) {
            switchTimestamp = newModel.getSwitchTimestamp();
            sensors.clear();
        }
        if (newModel.isForcedMode() != null) {
            forcedMode = newModel.isForcedMode();
        }
        if (newModel.getForcedModeTimestamp() != UNDEFINED_DATE) {
            forcedModeTimestamp = newModel.getForcedModeTimestamp();
        }
        for (int i = 0; i < newModel.getSensors().size(); i++) {
            sensors.put(newModel.getSensors().keyAt(i), newModel.getSensors().valueAt(i));
        }
        for (int i = 0; i < newModel.getSensorsThresholds().size(); i++) {
            sensorsThresholds.put(newModel.getSensorsThresholds().keyAt(i), newModel.getSensorsThresholds().valueAt(i));
        }
    }

    @Override
    public boolean isInitialized() {
        return state != null;
    }

    public int getId() {
        return id;
    }

    public void setId(int id) {
        this.id = id;
    }

    public Boolean getState() {
        return state;
    }

    public void setState(boolean state) {
        this.state = state;
    }

    public Date getSwitchTimestamp() {
        return switchTimestamp;
    }

    public void setSwitchTimestamp(Date switchTimestamp) {
        this.switchTimestamp = switchTimestamp;
    }

    public Boolean isForcedMode() {
        return forcedMode;
    }

    public void setForcedMode(boolean forcedMode) {
        this.forcedMode = forcedMode;
    }

    public Date getForcedModeTimestamp() {
        return forcedModeTimestamp;
    }

    public void setForcedModeTimestamp(Date forcedModeTimestamp) {
        this.forcedModeTimestamp = forcedModeTimestamp;
    }

    public SparseArray<ValueSensorModel> getSensors() {
        return sensors;
    }

    public void setSensors(SparseArray<ValueSensorModel> sensors) {
        this.sensors = sensors;
    }

    public void addSensor(ValueSensorModel sensor) {
        this.sensors.put(sensor.getId(), sensor);
    }

    public SparseArray<ValueSensorModel> getSensorsThresholds() {
        return sensorsThresholds;
    }

    public void setSensorsThresholds(SparseArray<ValueSensorModel> sensorsThresholds) {
        this.sensorsThresholds = sensorsThresholds;
    }

    public void addSensorThreshold(ValueSensorModel sensor) {
        this.sensorsThresholds.put(sensor.getId(), sensor);
    }
}

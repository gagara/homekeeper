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
    private SparseArray<SensorModel> sensors = new SparseArray<SensorModel>();

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
                SensorModel sm = new SensorModel(ids[i]);
                sm.restoreState(bundle, prefix + "node_" + id + "_sensors_");
                sensors.put(ids[i], sm);
            }
        }
    }

    public void update(NodeModel newModel) {
        state = newModel.getState();
        switchTimestamp = newModel.getSwitchTimestamp();
        forcedMode = newModel.isForcedMode();
        forcedModeTimestamp = newModel.getForcedModeTimestamp();
        for (int i = 0; i < newModel.getSensors().size(); i++) {
            sensors.put(newModel.getSensors().keyAt(i), newModel.getSensors().valueAt(i));
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

    public boolean getState() {
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

    public boolean isForcedMode() {
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

    public SparseArray<SensorModel> getSensors() {
        return sensors;
    }

    public void setSensors(SparseArray<SensorModel> sensors) {
        this.sensors = sensors;
    }

    public void addSensor(SensorModel sensor) {
        this.sensors.put(sensor.getId(), sensor);
    }
}
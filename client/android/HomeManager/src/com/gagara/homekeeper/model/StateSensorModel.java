package com.gagara.homekeeper.model;

import static com.gagara.homekeeper.common.Constants.UNDEFINED_DATE;

import java.util.Date;

import android.os.Bundle;

public class StateSensorModel extends SensorModel implements Model {

    private int id;
    private Boolean state = null;
    private Date switchTimestamp = UNDEFINED_DATE;

    public StateSensorModel(int id) {
        this.id = id;
    }

    @Override
    public void saveState(Bundle bundle) {
        saveState(bundle, "");
    }

    @Override
    public void saveState(Bundle bundle, String prefix) {
        if (state != null) {
            bundle.putBoolean(prefix + "sensor_" + id + "_state", state);
        }
        bundle.putLong(prefix + "sensor_" + id + "_switchTs", switchTimestamp.getTime());
    }

    @Override
    public void restoreState(Bundle bundle) {
        restoreState(bundle, "");
    }

    @Override
    public void restoreState(Bundle bundle, String prefix) {
        if (bundle.containsKey(prefix + "sensor_" + id + "_state")) {
            state = bundle.getBoolean(prefix + "sensor_" + id + "_state");
        }
        switchTimestamp = new Date(bundle.getLong(prefix + "sensor_" + id + "_switchTs"));
    }

    public void update(StateSensorModel newModel) {
        if (newModel.getState() != null) {
            state = newModel.getState();
        }
        if (newModel.getSwitchTimestamp() != UNDEFINED_DATE && !switchTimestamp.equals(newModel.getSwitchTimestamp())) {
            switchTimestamp = newModel.getSwitchTimestamp();
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

    public void setState(Boolean state) {
        this.state = state;
    }

    public Date getSwitchTimestamp() {
        return switchTimestamp;
    }

    public void setSwitchTimestamp(Date switchTimestamp) {
        this.switchTimestamp = switchTimestamp;
    }

}

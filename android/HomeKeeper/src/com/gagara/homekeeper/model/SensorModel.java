package com.gagara.homekeeper.model;

import static com.gagara.homekeeper.common.Constants.UNDEFINED_SENSOR_VALUE;
import android.os.Bundle;

public class SensorModel implements Model {

    private int id;
    private int value = UNDEFINED_SENSOR_VALUE;
    private int prevValue = UNDEFINED_SENSOR_VALUE;

    public SensorModel(int id) {
        this.id = id;
    }

    public SensorModel(SensorModel prev, int value) {
        super();
        this.id = prev.getId();
        this.value = value;
        this.prevValue = prev.getValue();
    }

    @Override
    public void saveState(Bundle bundle) {
        saveState(bundle, "");
    }

    @Override
    public void saveState(Bundle bundle, String prefix) {
        bundle.putInt(prefix + "sensor_" + id + "_value", value);
        bundle.putInt(prefix + "sensor_" + id + "_prevValue", prevValue);
    }

    @Override
    public void restoreState(Bundle bundle) {
        restoreState(bundle, "");
    }

    @Override
    public void restoreState(Bundle bundle, String prefix) {
        value = bundle.getInt(prefix + "sensor_" + id + "_value");
        prevValue = bundle.getInt(prefix + "sensor_" + id + "_prevValue");
    }

    @Override
    public boolean isInitialized() {
        return value != UNDEFINED_SENSOR_VALUE;
    }

    public int getId() {
        return id;
    }

    public void setId(int id) {
        this.id = id;
    }

    public int getValue() {
        return value;
    }

    public void setValue(int value) {
        this.value = value;
    }

    public int getPrevValue() {
        return prevValue;
    }

    public void setPrevValue(int prevValue) {
        this.prevValue = prevValue;
    }
}

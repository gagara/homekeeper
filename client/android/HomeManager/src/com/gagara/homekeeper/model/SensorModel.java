package com.gagara.homekeeper.model;

import static com.gagara.homekeeper.common.Constants.UNDEFINED_DATE;
import static com.gagara.homekeeper.common.Constants.UNDEFINED_SENSOR_VALUE;
import static com.gagara.homekeeper.model.SensorModel.SensorType.TEMPERATURE;

import java.util.Date;

import android.os.Bundle;

import com.gagara.homekeeper.common.Constants;

public class SensorModel implements Model {

    private int id;
    private SensorType type = TEMPERATURE;
    private int value = UNDEFINED_SENSOR_VALUE;
    private int prevValue = UNDEFINED_SENSOR_VALUE;
    private Date timestamp = UNDEFINED_DATE;

    public enum SensorType {
        TEMPERATURE,
        HUMIDITY
    }

    public SensorModel(int id) {
        this.id = id;
    }

    public SensorModel(SensorModel prev, int value) {
        super();
        this.id = prev.getId();
        this.type = prev.getType();
        this.value = value;
        this.prevValue = prev.getValue();
    }

    public SensorModel(SensorModel prev, int value, Date timestamp) {
        this(prev, value);
        this.timestamp = timestamp;
    }

    @Override
    public void saveState(Bundle bundle) {
        saveState(bundle, "");
    }

    @Override
    public void saveState(Bundle bundle, String prefix) {
        bundle.putString(prefix + "sensor_" + id + "_type", type.name());
        bundle.putInt(prefix + "sensor_" + id + "_value", value);
        bundle.putInt(prefix + "sensor_" + id + "_prevValue", prevValue);
        bundle.putLong(prefix + "sensor_" + id + "_ts", timestamp.getTime());
    }

    @Override
    public void restoreState(Bundle bundle) {
        restoreState(bundle, "");
    }

    @Override
    public void restoreState(Bundle bundle, String prefix) {
        type = SensorType.valueOf(bundle.getString(prefix + "sensor_" + id + "_type"));
        value = bundle.getInt(prefix + "sensor_" + id + "_value");
        prevValue = bundle.getInt(prefix + "sensor_" + id + "_prevValue");
        timestamp = new Date(bundle.getLong(prefix + "sensor_" + id + "_ts"));
    }

    public void update(SensorModel newModel) {
        if (isInitialized() && value != Constants.UNDEFINED_SENSOR_VALUE) {
            prevValue = value;
        }
        value = newModel.getValue();
        timestamp = newModel.getTimestamp();
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

    public SensorType getType() {
        return type;
    }

    public void setType(SensorType type) {
        this.type = type;
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

    public Date getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(Date timestamp) {
        this.timestamp = timestamp;
    }
}

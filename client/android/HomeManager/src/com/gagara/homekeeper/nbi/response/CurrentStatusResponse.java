package com.gagara.homekeeper.nbi.response;

import java.util.ArrayList;
import java.util.List;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.nbi.MessageHeader;

public class CurrentStatusResponse extends MessageHeader implements Response, Parcelable {

    private static final String TAG = CurrentStatusResponse.class.getName();

    private NodeStatusResponse node = null;
    private SensorStatusResponse sensor = null;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        super.writeToParcel(out, flags);
        List<String> content = new ArrayList<String>();
        if (node != null) {
            content.add("node");
        }
        if (sensor != null) {
            content.add("sensor");
        }
        out.writeStringList(content);
        if (node != null) {
            node.writeToParcel(out, flags);
        }
        if (sensor != null) {
            node.writeToParcel(out, flags);
        }
    }

    public static final Parcelable.Creator<CurrentStatusResponse> CREATOR = new Parcelable.Creator<CurrentStatusResponse>() {
        public CurrentStatusResponse createFromParcel(Parcel in) {
            return new CurrentStatusResponse(in);
        }

        public CurrentStatusResponse[] newArray(int size) {
            return new CurrentStatusResponse[size];
        }
    };

    public CurrentStatusResponse(long clocksDelta) {
        super();
        this.clocksDelta = clocksDelta;
    }

    public CurrentStatusResponse(Parcel in) {
        super(in);
        List<String> content = new ArrayList<String>();
        in.readStringList(content);
        if (content.contains("node")) {
            node = new NodeStatusResponse(in);
        }
        if (content.contains("sensor")) {
            sensor = new SensorStatusResponse(in);
        }
    }

    @Override
    public CurrentStatusResponse fromJson(JSONObject json) {
        if (clocksDelta == Long.MIN_VALUE) {
            throw new IllegalStateException("'clocksDelta' is not initialized");
        }
        try {
            if (ControllerConfig.MessageType.CURRENT_STATUS_REPORT == ControllerConfig.MessageType.forCode(json.get(
                    ControllerConfig.MSG_TYPE_KEY).toString())) {
                if (json.has(ControllerConfig.NODE_KEY)) {
                    JSONObject nodeJson = json.getJSONObject(ControllerConfig.NODE_KEY);
                    node = new NodeStatusResponse(clocksDelta);
                    node.fromJson(nodeJson);
                }
                if (json.has(ControllerConfig.SENSOR_KEY)) {
                    JSONObject sensorJson = json.getJSONObject(ControllerConfig.SENSOR_KEY);
                    sensor = new SensorStatusResponse(clocksDelta);
                    sensor.fromJson(sensorJson);
                }
                return this;
            } else {
                return null;
            }
        } catch (JSONException e) {
            Log.e(TAG, "failed to parse input message: " + e.getMessage(), e);
            return null;
        }
    }

    public NodeStatusResponse getNode() {
        return node;
    }

    public void setNode(NodeStatusResponse node) {
        this.node = node;
    }

    public SensorStatusResponse getSensor() {
        return sensor;
    }

    public void setSensor(SensorStatusResponse sensor) {
        this.sensor = sensor;
    }
}

package com.gagara.homekeeper.nbi.response;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;
import android.util.SparseArray;

import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.nbi.MessageHeader;

public class CurrentStatusResponse extends MessageHeader implements Response, Parcelable {

    private static final String TAG = CurrentStatusResponse.class.getName();

    private SparseArray<NodeStatusResponse> nodes = null;
    private SparseArray<SensorStatusResponse> sensors = null;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        super.writeToParcel(out, flags);
        out.writeIntArray(new int[] { nodes.size(), sensors.size() });
        for (int i = 0; i < nodes.size(); i++) {
            nodes.valueAt(i).writeToParcel(out, flags);
        }
        for (int i = 0; i < sensors.size(); i++) {
            sensors.valueAt(i).writeToParcel(out, flags);
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
        nodes = new SparseArray<NodeStatusResponse>();
        sensors = new SparseArray<SensorStatusResponse>();
    }

    public CurrentStatusResponse(Parcel in) {
        super(in);
        nodes = new SparseArray<NodeStatusResponse>();
        sensors = new SparseArray<SensorStatusResponse>();

        int[] counters = new int[2];
        in.readIntArray(counters);

        for (int i = 0; i < counters[0]; i++) {
            NodeStatusResponse node = new NodeStatusResponse(in);
            nodes.put(node.getData().getId(), node);
        }

        for (int i = 0; i < counters[1]; i++) {
            SensorStatusResponse sensor = new SensorStatusResponse(in);
            sensors.put(sensor.getData().getId(), sensor);
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
                    NodeStatusResponse node = new NodeStatusResponse(clocksDelta);
                    if (node.fromJson(nodeJson) != null) {
                        nodes.put(node.getData().getId(), node);
                    }
                }
                if (json.has(ControllerConfig.SENSOR_KEY)) {
                    JSONObject sensorJson = json.getJSONObject(ControllerConfig.SENSOR_KEY);
                    SensorStatusResponse sensor = new SensorStatusResponse(clocksDelta);
                    if (sensor.fromJson(sensorJson) != null) {
                        sensors.put(sensor.getData().getId(), sensor);

                    }
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

    public SparseArray<NodeStatusResponse> getNodes() {
        return nodes;
    }

    public void setNodes(SparseArray<NodeStatusResponse> nodes) {
        this.nodes = nodes;
    }

    public SparseArray<SensorStatusResponse> getSensors() {
        return sensors;
    }

    public void setSensors(SparseArray<SensorStatusResponse> sensors) {
        this.sensors = sensors;
    }
}

package com.gagara.homekeeper.nbi.response;

import java.util.Date;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.model.NodeModel;
import com.gagara.homekeeper.model.SensorModel;
import com.gagara.homekeeper.nbi.MessageHeader;

public class NodeStatusResponse extends MessageHeader implements Response, Parcelable {

    private static final String TAG = NodeStatusResponse.class.getName();

    private NodeModel data = null;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        super.writeToParcel(out, flags);
        if (data != null) {
            out.writeInt(data.getId());
            out.writeBooleanArray(new boolean[] { data.getState(), data.isForcedMode() });
            out.writeLongArray(new long[] { data.getSwitchTimestamp().getTime(),
                    data.getForcedModeTimestamp().getTime() });
            out.writeInt(data.getSensors().size());
            for (int i = 0; i < data.getSensors().size(); i++) {
                SensorModel s = data.getSensors().valueAt(i);
                out.writeIntArray(new int[] { s.getId(), s.getValue(), s.getPrevValue() });
            }
        } else {
            out.writeInt(0);
        }
    }

    public static final Parcelable.Creator<NodeStatusResponse> CREATOR = new Parcelable.Creator<NodeStatusResponse>() {
        public NodeStatusResponse createFromParcel(Parcel in) {
            return new NodeStatusResponse(in);
        }

        public NodeStatusResponse[] newArray(int size) {
            return new NodeStatusResponse[size];
        }
    };

    public NodeStatusResponse(long clocksDelta) {
        super();
        this.clocksDelta = clocksDelta;
    }

    public NodeStatusResponse(Parcel in) {
        super(in);
        int id = in.readInt();
        if (id > 0) {
            data = new NodeModel(id);
            boolean[] flags = new boolean[2];
            in.readBooleanArray(flags);
            data.setState(flags[0]);
            data.setForcedMode(flags[1]);
            long[] ts = new long[2];
            in.readLongArray(ts);
            data.setSwitchTimestamp(new Date(ts[0]));
            data.setForcedModeTimestamp(new Date(ts[1]));
            int sCnt = in.readInt();
            for (int i = 0; i < sCnt; i++) {
                int[] sv = new int[3];
                in.readIntArray(sv);
                SensorModel s = new SensorModel(sv[0]);
                s.setValue(sv[1]);
                s.setPrevValue(sv[2]);
                data.addSensor(s);
            }
        }
    }

    @Override
    public NodeStatusResponse fromJson(JSONObject json) {
        if (clocksDelta == Long.MIN_VALUE) {
            throw new IllegalStateException("'clocksDelta' is not initialized");
        }
        try {
            data = new NodeModel(json.getInt(ControllerConfig.ID_KEY));
            data.setState(json.getInt(ControllerConfig.STATE_KEY) == 1 ? true : false);
            if (json.getLong(ControllerConfig.TIMESTAMP_KEY) != 0) {
                data.setSwitchTimestamp(new Date(json.getLong(ControllerConfig.TIMESTAMP_KEY) + clocksDelta));
            }
            data.setForcedMode(json.getInt(ControllerConfig.FORCE_FLAG_KEY) == 1 ? true : false);
            if (json.has(ControllerConfig.FORCE_TIMESTAMP_KEY)
                    && json.getLong(ControllerConfig.FORCE_TIMESTAMP_KEY) != 0) {
                data.setForcedModeTimestamp(new Date(json.getLong(ControllerConfig.FORCE_TIMESTAMP_KEY) + clocksDelta));
            }
            if (json.has(ControllerConfig.SENSORS_KEY)) {
                JSONArray sensorsJson = json.getJSONArray(ControllerConfig.SENSORS_KEY);
                for (int i = 0; i < sensorsJson.length(); i++) {
                    JSONObject sensorJson = sensorsJson.getJSONObject(i);
                    int id = sensorJson.getInt(ControllerConfig.ID_KEY);
                    int value = sensorJson.getInt(ControllerConfig.VALUE_KEY);
                    SensorModel sensor = new SensorModel(id);
                    sensor.setValue(value);
                    data.addSensor(sensor);
                }
            }
        } catch (JSONException e) {
            Log.e(TAG, "failed to parse input message: " + e.getMessage(), e);
            return null;
        }
        return this;
    }

    public NodeModel getData() {
        return data;
    }

    public void setData(NodeModel data) {
        this.data = data;
    }
}
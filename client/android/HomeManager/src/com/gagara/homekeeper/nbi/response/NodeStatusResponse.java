package com.gagara.homekeeper.nbi.response;

import static com.gagara.homekeeper.common.ControllerConfig.FORCE_FLAG_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.FORCE_TIMESTAMP_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.ID_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.STATE_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.TIMESTAMP_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.VALUE_KEY;

import java.util.Date;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;

import com.gagara.homekeeper.model.NodeModel;
import com.gagara.homekeeper.model.ValueSensorModel;
import com.gagara.homekeeper.nbi.MessageHeader;

public class NodeStatusResponse extends MessageHeader implements Response, Parcelable {

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
                ValueSensorModel s = data.getSensors().valueAt(i);
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

    public NodeStatusResponse() {
        super();
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
                ValueSensorModel s = new ValueSensorModel(sv[0]);
                s.setValue(sv[1]);
                s.setPrevValue(sv[2]);
                data.addSensor(s);
            }
        }
    }

    @Override
    public NodeStatusResponse fromJson(JSONObject json) throws JSONException {
        data = new NodeModel(json.getInt(ID_KEY));
        data.setState(json.getInt(STATE_KEY) == 1 ? true : false);
        if (json.getLong(TIMESTAMP_KEY) != 0) {
            data.setSwitchTimestamp(new Date(json.getLong(TIMESTAMP_KEY) * 1000));
        }
        data.setForcedMode(json.getInt(FORCE_FLAG_KEY) == 1 ? true : false);
        if (json.has(FORCE_TIMESTAMP_KEY) && json.getLong(FORCE_TIMESTAMP_KEY) != 0) {
            data.setForcedModeTimestamp(new Date(json.getLong(FORCE_TIMESTAMP_KEY) * 1000));
        }
        if (json.has(SENSOR_KEY)) {
            JSONArray sensorsJson = json.getJSONArray(SENSOR_KEY);
            for (int i = 0; i < sensorsJson.length(); i++) {
                JSONObject sensorJson = sensorsJson.getJSONObject(i);
                int id = sensorJson.getInt(ID_KEY);
                int value = sensorJson.getInt(VALUE_KEY);
                ValueSensorModel sensor = new ValueSensorModel(id);
                sensor.setValue(value);
                data.addSensor(sensor);
            }
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

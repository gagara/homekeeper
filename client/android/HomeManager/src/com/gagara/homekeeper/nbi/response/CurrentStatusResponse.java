package com.gagara.homekeeper.nbi.response;

import static com.gagara.homekeeper.common.ControllerConfig.ID_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.MSG_TYPE_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_KEY;

import java.util.ArrayList;
import java.util.List;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.nbi.MessageHeader;
import com.gagara.homekeeper.ui.view.ViewUtils;

public class CurrentStatusResponse extends MessageHeader implements Response, Parcelable {

    private static final String TAG = CurrentStatusResponse.class.getName();

    private NodeStatusResponse node = null;
    private ValueSensorStatusResponse valueSensor = null;
    private StateSensorStatusResponse stateSensor = null;

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
        if (valueSensor != null) {
            content.add("valueSensor");
        }
        if (stateSensor != null) {
            content.add("stateSensor");
        }
        out.writeStringList(content);
        if (node != null) {
            node.writeToParcel(out, flags);
        }
        if (valueSensor != null) {
            valueSensor.writeToParcel(out, flags);
        }
        if (stateSensor != null) {
            stateSensor.writeToParcel(out, flags);
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
        if (content.contains("valueSensor")) {
            valueSensor = new ValueSensorStatusResponse(in);
        }
        if (content.contains("stateSensor")) {
            stateSensor = new StateSensorStatusResponse(in);
        }
    }

    @Override
    public CurrentStatusResponse fromJson(JSONObject json) throws JSONException {
        if (clocksDelta == Long.MIN_VALUE) {
            throw new IllegalStateException("'clocksDelta' is not initialized");
        }
        if (ControllerConfig.MessageType.CURRENT_STATUS_REPORT == ControllerConfig.MessageType.forCode(json.get(
                MSG_TYPE_KEY).toString())) {
            if (json.has(NODE_KEY)) {
                JSONObject nodeJson = json.getJSONObject(NODE_KEY);
                node = new NodeStatusResponse(clocksDelta);
                node.fromJson(nodeJson);
            }
            if (json.has(SENSOR_KEY)) {
                JSONObject sensorJson = json.getJSONObject(SENSOR_KEY);
                int id = sensorJson.getInt(ID_KEY);
                if (ViewUtils.validValueSensorId(id)) {
                    valueSensor = new ValueSensorStatusResponse(clocksDelta);
                    valueSensor.fromJson(sensorJson);
                } else {
                    stateSensor = new StateSensorStatusResponse(clocksDelta);
                    stateSensor.fromJson(sensorJson);
                }
            }
            return this;
        } else {
            return null;
        }
    }

    public NodeStatusResponse getNode() {
        return node;
    }

    public void setNode(NodeStatusResponse node) {
        this.node = node;
    }

    public ValueSensorStatusResponse getValueSensor() {
        return valueSensor;
    }

    public void setValueSensor(ValueSensorStatusResponse sensor) {
        this.valueSensor = sensor;
    }

    public StateSensorStatusResponse getStateSensor() {
        return stateSensor;
    }

    public void setStateSensor(StateSensorStatusResponse sensor) {
        this.stateSensor = sensor;
    }
}

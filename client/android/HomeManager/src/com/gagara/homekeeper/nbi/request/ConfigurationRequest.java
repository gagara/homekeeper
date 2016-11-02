package com.gagara.homekeeper.nbi.request;

import static com.gagara.homekeeper.common.ControllerConfig.ID_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.MSG_TYPE_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.VALUE_KEY;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.nbi.MessageHeader;

public class ConfigurationRequest extends MessageHeader implements Request, Parcelable {

    private static final String TAG = ConfigurationRequest.class.getName();

    private Integer sensorId = null;
    private Integer sensorThreshold = null;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        super.writeToParcel(out, flags);
    }

    public static final Parcelable.Creator<ConfigurationRequest> CREATOR = new Parcelable.Creator<ConfigurationRequest>() {
        public ConfigurationRequest createFromParcel(Parcel in) {
            return new ConfigurationRequest(in);
        }

        public ConfigurationRequest[] newArray(int size) {
            return new ConfigurationRequest[size];
        }
    };

    public ConfigurationRequest() {
        super();
    }

    public ConfigurationRequest(Parcel in) {
        super(in);
    }

    @Override
    public JSONObject toJson() {
        JSONObject json = new JSONObject();
        try {
            json.put(MSG_TYPE_KEY, ControllerConfig.MessageType.CONFIGURATION.code());

            if (sensorId != null && sensorThreshold != null) {
                JSONObject sensor = new JSONObject();
                sensor.put(ID_KEY, sensorId);
                sensor.put(VALUE_KEY, sensorThreshold);
                json.put(SENSOR_KEY, sensor);
            }
            if (sensorThreshold != null) {
            }
        } catch (JSONException e) {
            Log.e(TAG, "failed to serialize to JSON: " + this.toString() + ": " + e.getMessage(), e);
            return null;
        }
        return json;
    }

    public Integer getSensorId() {
        return sensorId;
    }

    public void setSensorId(Integer sensorId) {
        this.sensorId = sensorId;
    }

    public Integer getSensorThreshold() {
        return sensorThreshold;
    }

    public void setSensorThreshold(Integer sensorThreshold) {
        this.sensorThreshold = sensorThreshold;
    }
}

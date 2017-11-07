package com.gagara.homekeeper.nbi.response;

import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.VALUE_KEY;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.nbi.MessageHeader;

public class ConfigurationResponse extends MessageHeader implements Response, Parcelable {

    private static final String TAG = ConfigurationResponse.class.getName();

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        super.writeToParcel(out, flags);
    }

    public static final Parcelable.Creator<ConfigurationResponse> CREATOR = new Parcelable.Creator<ConfigurationResponse>() {
        public ConfigurationResponse createFromParcel(Parcel in) {
            return new ConfigurationResponse(in);
        }

        public ConfigurationResponse[] newArray(int size) {
            return new ConfigurationResponse[size];
        }
    };

    public ConfigurationResponse(long clocksDelta) {
        super();
        this.clocksDelta = clocksDelta;
    }

    public ConfigurationResponse(Parcel in) {
        super(in);
    }

    @Override
    public ConfigurationResponse fromJson(JSONObject json) throws JSONException {
        if (ControllerConfig.MessageType.CONFIGURATION == ControllerConfig.MessageType.forCode(json.get(
                ControllerConfig.MSG_TYPE_KEY).toString())) {
            if (json.has(SENSOR_KEY) && json.getJSONObject(SENSOR_KEY).has(VALUE_KEY)) {
                ConfigurationResponse response = new SensorThresholdConfigurationResponse(clocksDelta);
                return response.fromJson(json);
            }
        }
        return null;
    }
}

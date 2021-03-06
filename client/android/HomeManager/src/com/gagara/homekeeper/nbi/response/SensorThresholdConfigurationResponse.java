package com.gagara.homekeeper.nbi.response;

import static com.gagara.homekeeper.common.ControllerConfig.ID_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.VALUE_KEY;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;

import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.model.ValueSensorModel;

public class SensorThresholdConfigurationResponse extends ConfigurationResponse implements Response, Parcelable {

    private ValueSensorModel data = null;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        super.writeToParcel(out, flags);
        if (data != null) {
            out.writeInt(data.getId());
            out.writeInt(data.getValue());
        } else {
            out.writeInt(0);
        }
    }

    public static final Parcelable.Creator<SensorThresholdConfigurationResponse> CREATOR = new Parcelable.Creator<SensorThresholdConfigurationResponse>() {
        public SensorThresholdConfigurationResponse createFromParcel(Parcel in) {
            return new SensorThresholdConfigurationResponse(in);
        }

        public SensorThresholdConfigurationResponse[] newArray(int size) {
            return new SensorThresholdConfigurationResponse[size];
        }
    };

    public SensorThresholdConfigurationResponse() {
        super();
    }

    public SensorThresholdConfigurationResponse(Parcel in) {
        super(in);
        int id = in.readInt();
        if (id > 0) {
            data = new ValueSensorModel(id);
            data.setValue(in.readInt());
        }
    }

    @Override
    public SensorThresholdConfigurationResponse fromJson(JSONObject json) throws JSONException {
        JSONObject sensor = json.getJSONObject(ControllerConfig.SENSOR_KEY);
        int id = sensor.getInt(ID_KEY);
        int value = sensor.getInt(VALUE_KEY);
        data = new ValueSensorModel(id);
        data.setValue(value);
        return this;
    }

    public ValueSensorModel getData() {
        return data;
    }

    public void setData(ValueSensorModel data) {
        this.data = data;
    }
}

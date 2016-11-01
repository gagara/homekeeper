package com.gagara.homekeeper.nbi.response;

import static com.gagara.homekeeper.common.ControllerConfig.ID_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.VALUE_KEY;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.model.SensorModel;

public class SensorThresholdConfigurationResponse extends ConfigurationResponse implements Response, Parcelable {

    private static final String TAG = SensorThresholdConfigurationResponse.class.getName();

    private SensorModel data = null;

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

    public SensorThresholdConfigurationResponse(long clocksDelta) {
        super(clocksDelta);
    }

    public SensorThresholdConfigurationResponse(Parcel in) {
        super(in);
        int id = in.readInt();
        if (id > 0) {
            data = new SensorModel(id);
            data.setValue(in.readInt());
        }
    }

    @Override
    public SensorThresholdConfigurationResponse fromJson(JSONObject json) {
        try {
            int id = json.getInt(ID_KEY);
            int value = json.getInt(VALUE_KEY);
            data = new SensorModel(id);
            data.setValue(value);
        } catch (JSONException e) {
            Log.e(TAG, "failed to parse input message: " + e.getMessage(), e);
            return null;
        }
        return this;
    }

    public SensorModel getData() {
        return data;
    }

    public void setData(SensorModel data) {
        this.data = data;
    }
}

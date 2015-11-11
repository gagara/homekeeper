package com.gagara.homekeeper.nbi.response;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.model.SensorModel;
import com.gagara.homekeeper.nbi.MessageHeader;

public class SensorStatusResponse extends MessageHeader implements Response, Parcelable {

    private static final String TAG = SensorStatusResponse.class.getName();

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
            out.writeInt(data.getPrevValue());
        } else {
            out.writeInt(0);
        }
    }

    public static final Parcelable.Creator<SensorStatusResponse> CREATOR = new Parcelable.Creator<SensorStatusResponse>() {
        public SensorStatusResponse createFromParcel(Parcel in) {
            return new SensorStatusResponse(in);
        }

        public SensorStatusResponse[] newArray(int size) {
            return new SensorStatusResponse[size];
        }
    };

    public SensorStatusResponse(long clocksDelta) {
        super();
        this.clocksDelta = clocksDelta;
    }

    public SensorStatusResponse(Parcel in) {
        super(in);
        int id = in.readInt();
        if (id > 0) {
            data = new SensorModel(id);
            data.setValue(in.readInt());
            data.setPrevValue(in.readInt());
        }
    }

    @Override
    public SensorStatusResponse fromJson(JSONObject json) {
        try {
            int id = json.getInt(ControllerConfig.ID_KEY);
            int value = json.getInt(ControllerConfig.VALUE_KEY);
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
package com.gagara.homekeeper.nbi.response;

import static com.gagara.homekeeper.common.Constants.UNDEFINED_DATE;
import static com.gagara.homekeeper.common.ControllerConfig.ID_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.TIMESTAMP_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.VALUE_KEY;

import java.util.Date;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.model.ValueSensorModel;
import com.gagara.homekeeper.nbi.MessageHeader;

public class ValueSensorStatusResponse extends MessageHeader implements Response, Parcelable {

    private static final String TAG = ValueSensorStatusResponse.class.getName();

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
            out.writeInt(data.getPrevValue());
        } else {
            out.writeInt(0);
        }
    }

    public static final Parcelable.Creator<ValueSensorStatusResponse> CREATOR = new Parcelable.Creator<ValueSensorStatusResponse>() {
        public ValueSensorStatusResponse createFromParcel(Parcel in) {
            return new ValueSensorStatusResponse(in);
        }

        public ValueSensorStatusResponse[] newArray(int size) {
            return new ValueSensorStatusResponse[size];
        }
    };

    public ValueSensorStatusResponse(long clocksDelta) {
        super();
        this.clocksDelta = clocksDelta;
    }

    public ValueSensorStatusResponse(Parcel in) {
        super(in);
        int id = in.readInt();
        if (id > 0) {
            data = new ValueSensorModel(id);
            data.setValue(in.readInt());
            data.setPrevValue(in.readInt());
        }
    }

    @Override
    public ValueSensorStatusResponse fromJson(JSONObject json) throws JSONException {
        int id = json.getInt(ID_KEY);
        int value = json.getInt(VALUE_KEY);
        Date ts = UNDEFINED_DATE;
        if (json.has(TIMESTAMP_KEY) && json.getLong(TIMESTAMP_KEY) != 0) {
            ts = new Date((json.getLong(TIMESTAMP_KEY) + clocksDelta) * 1000);
        }
        data = new ValueSensorModel(id);
        data.setValue(value);
        data.setTimestamp(ts);
        return this;
    }

    public ValueSensorModel getData() {
        return data;
    }

    public void setData(ValueSensorModel data) {
        this.data = data;
    }
}
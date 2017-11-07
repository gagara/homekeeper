package com.gagara.homekeeper.nbi.response;

import static com.gagara.homekeeper.common.ControllerConfig.ID_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.TIMESTAMP_KEY;
import static com.gagara.homekeeper.common.ControllerConfig.VALUE_KEY;

import java.util.Date;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.model.StateSensorModel;
import com.gagara.homekeeper.nbi.MessageHeader;

public class StateSensorStatusResponse extends MessageHeader implements Response, Parcelable {

    private static final String TAG = StateSensorStatusResponse.class.getName();

    private StateSensorModel data = null;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        super.writeToParcel(out, flags);
        if (data != null) {
            out.writeInt(data.getId());
            out.writeBooleanArray(new boolean[] { data.getState() });
            out.writeLongArray(new long[] { data.getSwitchTimestamp().getTime() });
        } else {
            out.writeInt(0);
        }
    }

    public static final Parcelable.Creator<StateSensorStatusResponse> CREATOR = new Parcelable.Creator<StateSensorStatusResponse>() {
        public StateSensorStatusResponse createFromParcel(Parcel in) {
            return new StateSensorStatusResponse(in);
        }

        public StateSensorStatusResponse[] newArray(int size) {
            return new StateSensorStatusResponse[size];
        }
    };

    public StateSensorStatusResponse(long clocksDelta) {
        super();
        this.clocksDelta = clocksDelta;
    }

    public StateSensorStatusResponse(Parcel in) {
        super(in);
        int id = in.readInt();
        if (id > 0) {
            data = new StateSensorModel(id);
            boolean[] flags = new boolean[1];
            in.readBooleanArray(flags);
            data.setState(flags[0]);
            long[] ts = new long[1];
            in.readLongArray(ts);
            data.setSwitchTimestamp(new Date(ts[0]));
        }
    }

    @Override
    public StateSensorStatusResponse fromJson(JSONObject json) throws JSONException {
        data = new StateSensorModel(json.getInt(ID_KEY));
        data.setState(json.getInt(VALUE_KEY) == 1 ? true : false);
        if (json.getLong(TIMESTAMP_KEY) != 0) {
            data.setSwitchTimestamp(new Date((json.getLong(TIMESTAMP_KEY) + clocksDelta) * 1000));
        }
        return this;
    }

    public StateSensorModel getData() {
        return data;
    }

    public void setData(StateSensorModel data) {
        this.data = data;
    }
}
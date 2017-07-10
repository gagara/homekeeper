package com.gagara.homekeeper.nbi.response;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.nbi.MessageHeader;

public class FastClockSyncResponse extends MessageHeader implements Response, Parcelable {

    private static final String TAG = FastClockSyncResponse.class.getName();

    private long clockSeconds;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        super.writeToParcel(out, flags);
        out.writeLong(clockSeconds);
    }

    public static final Parcelable.Creator<FastClockSyncResponse> CREATOR = new Parcelable.Creator<FastClockSyncResponse>() {
        public FastClockSyncResponse createFromParcel(Parcel in) {
            return new FastClockSyncResponse(in);
        }

        public FastClockSyncResponse[] newArray(int size) {
            return new FastClockSyncResponse[size];
        }
    };

    public FastClockSyncResponse() {
        super();
    }

    public FastClockSyncResponse(Parcel in) {
        super(in);
        clockSeconds = in.readLong();
    }

    @Override
    public FastClockSyncResponse fromJson(JSONObject json) {
        try {
            if (ControllerConfig.MessageType.FAST_CLOCK_SYNC == ControllerConfig.MessageType
                    .forCode(json.get(ControllerConfig.MSG_TYPE_KEY).toString())) {
                this.setClockSeconds(json.getLong(ControllerConfig.TIMESTAMP_KEY));
            }
        } catch (JSONException e) {
            Log.e(TAG, "failed to parse input message: " + e.getMessage(), e);
            return null;
        }
        return this;
    }

    public long getClockSeconds() {
        return clockSeconds;
    }

    public void setClockSeconds(long clockSeconds) {
        this.clockSeconds = clockSeconds;
    }
}

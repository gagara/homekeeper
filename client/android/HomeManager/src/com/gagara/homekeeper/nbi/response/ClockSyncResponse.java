package com.gagara.homekeeper.nbi.response;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.nbi.MessageHeader;

public class ClockSyncResponse extends MessageHeader implements Response, Parcelable {

    private static final String TAG = ClockSyncResponse.class.getName();

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

    public static final Parcelable.Creator<ClockSyncResponse> CREATOR = new Parcelable.Creator<ClockSyncResponse>() {
        public ClockSyncResponse createFromParcel(Parcel in) {
            return new ClockSyncResponse(in);
        }

        public ClockSyncResponse[] newArray(int size) {
            return new ClockSyncResponse[size];
        }
    };

    public ClockSyncResponse() {
        super();
    }

    public ClockSyncResponse(Parcel in) {
        super(in);
        clockSeconds = in.readLong();
    }

    @Override
    public ClockSyncResponse fromJson(JSONObject json) {
        try {
            if (ControllerConfig.MessageType.CLOCK_SYNC == ControllerConfig.MessageType
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

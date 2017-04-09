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

    private long clockMillis;
    private int overflowCount;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        super.writeToParcel(out, flags);
        out.writeLong(clockMillis);
        out.writeInt(overflowCount);
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
        clockMillis = in.readLong();
        overflowCount = in.readInt();
    }

    @Override
    public FastClockSyncResponse fromJson(JSONObject json) {
        try {
            if (ControllerConfig.MessageType.FAST_CLOCK_SYNC == ControllerConfig.MessageType
                    .forCode(json.get(ControllerConfig.MSG_TYPE_KEY).toString())) {
                this.setClockMillis(json.getLong(ControllerConfig.TIMESTAMP_KEY));
                this.setOverflowCount(json.getInt(ControllerConfig.OVERFLOW_COUNT_KEY));
            }
        } catch (JSONException e) {
            Log.e(TAG, "failed to parse input message: " + e.getMessage(), e);
            return null;
        }
        return this;
    }

    public long getClockMillis() {
        return clockMillis;
    }

    public void setClockMillis(long clockMillis) {
        this.clockMillis = clockMillis;
    }

    public int getOverflowCount() {
        return overflowCount;
    }

    public void setOverflowCount(int overflowCount) {
        this.overflowCount = overflowCount;
    }
}
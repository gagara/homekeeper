package com.gagara.homekeeper.nbi.request;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.nbi.MessageHeader;

public class FastClockSyncRequest extends MessageHeader implements Request, Parcelable {

    private static final String TAG = FastClockSyncRequest.class.getName();

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        super.writeToParcel(out, flags);
    }

    public static final Parcelable.Creator<FastClockSyncRequest> CREATOR = new Parcelable.Creator<FastClockSyncRequest>() {
        public FastClockSyncRequest createFromParcel(Parcel in) {
            return new FastClockSyncRequest(in);
        }

        public FastClockSyncRequest[] newArray(int size) {
            return new FastClockSyncRequest[size];
        }
    };

    public FastClockSyncRequest() {
        super();
    }

    public FastClockSyncRequest(Parcel in) {
        super(in);
    }

    @Override
    public JSONObject toJson() {
        JSONObject json = new JSONObject();
        try {
            json.put(ControllerConfig.MSG_TYPE_KEY, ControllerConfig.MessageType.FAST_CLOCK_SYNC.code());
        } catch (JSONException e) {
            Log.e(TAG, "failed to serialize to JSON: " + this.toString() + ": " + e.getMessage(), e);
            return null;
        }
        return json;
    }
}
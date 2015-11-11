package com.gagara.homekeeper.nbi.request;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.nbi.MessageHeader;

public class CurrentStatusRequest extends MessageHeader implements Request, Parcelable {

    private static final String TAG = CurrentStatusRequest.class.getName();

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        super.writeToParcel(out, flags);
    }

    public static final Parcelable.Creator<CurrentStatusRequest> CREATOR = new Parcelable.Creator<CurrentStatusRequest>() {
        public CurrentStatusRequest createFromParcel(Parcel in) {
            return new CurrentStatusRequest(in);
        }

        public CurrentStatusRequest[] newArray(int size) {
            return new CurrentStatusRequest[size];
        }
    };

    public CurrentStatusRequest() {
        super(0);
    }

    public CurrentStatusRequest(Parcel in) {
        super(in);
    }

    @Override
    public JSONObject toJson() {
        JSONObject json = new JSONObject();
        try {
            json.put(ControllerConfig.MSG_TYPE_KEY, ControllerConfig.MessageType.CURRENT_STATUS_REPORT.code());
        } catch (JSONException e) {
            Log.e(TAG, "failed to serialize to JSON: " + this.toString() + ": " + e.getMessage(), e);
            return null;
        }
        return json;
    }
}

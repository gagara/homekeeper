package com.gagara.homekeeper.nbi.request;

import java.util.Date;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.nbi.MessageHeader;

public class LogRequest extends MessageHeader implements Request, Parcelable {

    private static final String TAG = LogRequest.class.getName();

    private Date timestamp = null;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        super.writeToParcel(out, flags);
        out.writeLong(timestamp.getTime());
    }

    public static final Parcelable.Creator<LogRequest> CREATOR = new Parcelable.Creator<LogRequest>() {
        public LogRequest createFromParcel(Parcel in) {
            return new LogRequest(in);
        }

        public LogRequest[] newArray(int size) {
            return new LogRequest[size];
        }
    };

    public LogRequest() {
        super();
        timestamp = new Date();
    }

    public LogRequest(Date timestamp) {
        super();
        this.timestamp = timestamp;
    }

    public LogRequest(Parcel in) {
        super(in);
        timestamp = new Date(in.readLong());
    }

    @Override
    public JSONObject toJson() {
        JSONObject json = new JSONObject();
        try {
            json.put(ControllerConfig.MSG_TYPE_KEY, ControllerConfig.MessageType.LOG.code());
            json.put(ControllerConfig.TIMESTAMP_KEY, timestamp.getTime() / 1000);
        } catch (JSONException e) {
            Log.e(TAG, "failed to serialize to JSON: " + this.toString() + ": " + e.getMessage(), e);
            return null;
        }
        return json;
    }

    public Date getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(Date timestamp) {
        this.timestamp = timestamp;
    }
}

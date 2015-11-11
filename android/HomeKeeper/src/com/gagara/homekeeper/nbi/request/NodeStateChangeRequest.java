package com.gagara.homekeeper.nbi.request;

import java.util.Date;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.nbi.MessageHeader;

public class NodeStateChangeRequest extends MessageHeader implements Request, Parcelable {

    private static final String TAG = NodeStateChangeRequest.class.getName();

    private int id;
    private Boolean state = null;
    private Date validityTimestamp = null;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        super.writeToParcel(out, flags);
        out.writeInt(id);
        int i = 0;
        if (state != null) {
            i++;
        }
        if (validityTimestamp != null) {
            i++;
        }
        out.writeInt(i);
        if (state != null) {
            out.writeBooleanArray(new boolean[] { state });
        }
        if (validityTimestamp != null) {
            out.writeLong(validityTimestamp.getTime());
        }
    }

    public static final Parcelable.Creator<NodeStateChangeRequest> CREATOR = new Parcelable.Creator<NodeStateChangeRequest>() {
        public NodeStateChangeRequest createFromParcel(Parcel in) {
            return new NodeStateChangeRequest(in);
        }

        public NodeStateChangeRequest[] newArray(int size) {
            return new NodeStateChangeRequest[size];
        }
    };

    public NodeStateChangeRequest(long clockDelta) {
        super(clockDelta);
    }

    public NodeStateChangeRequest(Parcel in) {
        super(in);
        id = in.readInt();
        int i = in.readInt();
        if (i > 0) {
            boolean[] states = new boolean[1];
            in.readBooleanArray(states);
            state = states[0];
        }
        if (i > 1) {
            validityTimestamp = new Date(in.readLong());
        }
    }

    @Override
    public JSONObject toJson() {
        JSONObject json = new JSONObject();
        try {
            json.put(ControllerConfig.MSG_TYPE_KEY, ControllerConfig.MessageType.NODE_STATE_CHANGED.code());
            json.put(ControllerConfig.ID_KEY, id);
            if (state != null) {
                json.put(ControllerConfig.STATE_KEY, state ? "1" : "0");
                if (validityTimestamp != null) {
                    json.put(ControllerConfig.FORCE_TIMESTAMP_KEY, (validityTimestamp.getTime() - clocksDelta) % Math.pow(2, 32));
                }
            }
        } catch (JSONException e) {
            Log.e(TAG, "failed to serialize to JSON: " + this.toString() + ": " + e.getMessage(), e);
            return null;
        }
        return json;
    }

    public int getId() {
        return id;
    }

    public void setId(int id) {
        this.id = id;
    }

    public Boolean getState() {
        return state;
    }

    public void setState(Boolean state) {
        this.state = state;
    }

    public Date getValidityTimestamp() {
        return validityTimestamp;
    }

    public void setValidityTimestamp(Date validityTimestamp) {
        this.validityTimestamp = validityTimestamp;
    }
}

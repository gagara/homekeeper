package com.gagara.homekeeper.nbi.response;

import org.json.JSONException;
import org.json.JSONObject;

import android.annotation.SuppressLint;
import android.os.Parcel;
import android.os.Parcelable;

import com.gagara.homekeeper.common.ControllerConfig;

@SuppressLint("UseSparseArrays")
public class NodeStateChangeResponse extends NodeStatusResponse implements Response, Parcelable {

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        super.writeToParcel(out, flags);
    }

    public static final Parcelable.Creator<NodeStateChangeResponse> CREATOR = new Parcelable.Creator<NodeStateChangeResponse>() {
        public NodeStateChangeResponse createFromParcel(Parcel in) {
            return new NodeStateChangeResponse(in);
        }

        public NodeStateChangeResponse[] newArray(int size) {
            return new NodeStateChangeResponse[size];
        }
    };

    public NodeStateChangeResponse() {
        super();
    }

    public NodeStateChangeResponse(Parcel in) {
        super(in);
    }

    @Override
    public NodeStateChangeResponse fromJson(JSONObject json) throws JSONException {
        if (ControllerConfig.MessageType.NODE_STATE_CHANGED == ControllerConfig.MessageType.forCode(json.get(
                ControllerConfig.MSG_TYPE_KEY).toString())) {
            return (NodeStateChangeResponse) super.fromJson(json);
        } else {
            return null;
        }
    }
}

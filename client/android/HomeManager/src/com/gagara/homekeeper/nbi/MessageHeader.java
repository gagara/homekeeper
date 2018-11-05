package com.gagara.homekeeper.nbi;

import java.util.Date;

import android.os.Parcel;

public abstract class MessageHeader {

    protected Date timestamp;

    public void writeToParcel(Parcel out, int flags) {
        out.writeLong(timestamp.getTime());
    }

    public MessageHeader() {
        this.timestamp = new Date();
    }

    public MessageHeader(Parcel in) {
        timestamp = new Date(in.readLong());
    }

    public Date getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(Date timestamp) {
        this.timestamp = timestamp;
    }
}

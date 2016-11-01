package com.gagara.homekeeper.nbi;

import java.util.Date;

import android.os.Parcel;

public abstract class MessageHeader {

    protected Date timestamp;
    protected long clocksDelta;

    public void writeToParcel(Parcel out, int flags) {
        out.writeLong(timestamp.getTime());
        out.writeLong(clocksDelta);
    }

    public MessageHeader() {
        this.timestamp = new Date();
        this.clocksDelta = Long.MIN_VALUE;
    }

    public MessageHeader(Parcel in) {
        timestamp = new Date(in.readLong());
        clocksDelta = in.readLong();
    }

    public Date getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(Date timestamp) {
        this.timestamp = timestamp;
    }

    public long getClocksDelta() {
        return clocksDelta;
    }

    public void setClocksDelta(long clocksDelta) {
        this.clocksDelta = clocksDelta;
    }
}

package com.gagara.homekeeper.model;

import java.util.Date;

import android.os.Bundle;

public class ServiceInfoModel implements Model {

    private Date timestamp = null;

    public ServiceInfoModel() {
    }

    public ServiceInfoModel(Date timestamp) {
        super();
        this.timestamp = timestamp;
    }

    @Override
    public void saveState(Bundle bundle) {
        saveState(bundle, "");
    }

    @Override
    public void saveState(Bundle bundle, String prefix) {
        if (timestamp != null) {
            bundle.putLong(prefix + "service_info_timestamp", timestamp.getTime());
        }
    }

    @Override
    public void restoreState(Bundle bundle) {
        restoreState(bundle, "");
    }

    @Override
    public void restoreState(Bundle bundle, String prefix) {
        long ts = bundle.getLong(prefix + "service_info_timestamp", 0L);
        if (ts != 0) {
            timestamp = new Date(ts);
        }
    }

    @Override
    public boolean isInitialized() {
        return timestamp != null;
    }

    public Date getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(Date timestamp) {
        this.timestamp = timestamp;
    }

}

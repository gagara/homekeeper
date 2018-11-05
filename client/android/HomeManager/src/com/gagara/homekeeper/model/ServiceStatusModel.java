package com.gagara.homekeeper.model;

import java.util.Date;

import android.os.Bundle;

public class ServiceStatusModel implements Model {

    private String details = null;
    private Date timestamp = null;

    public ServiceStatusModel() {
    }

    public ServiceStatusModel(String details, Date timestamp) {
        super();
        this.details = details;
        this.timestamp = timestamp;
    }

    @Override
    public void saveState(Bundle bundle) {
        saveState(bundle, "");
    }

    @Override
    public void saveState(Bundle bundle, String prefix) {
        bundle.putString(prefix + "service_status_details", details);
        if (timestamp != null) {
            bundle.putLong(prefix + "service_status_timestamp", timestamp.getTime());
        }
    }

    @Override
    public void restoreState(Bundle bundle) {
        restoreState(bundle, "");
    }

    @Override
    public void restoreState(Bundle bundle, String prefix) {
        details = bundle.getString(prefix + "service_status_details");
        Long ts = bundle.getLong(prefix + "service_status_timestamp");
        if (ts > 0) {
            this.timestamp = new Date(ts);
        }
    }

    @Override
    public boolean isInitialized() {
        return true;
    }

    public String getDetails() {
        return details;
    }

    public void setDetails(String details) {
        this.details = details;
    }

    public Date getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(Date timestamp) {
        this.timestamp = timestamp;
    }
}

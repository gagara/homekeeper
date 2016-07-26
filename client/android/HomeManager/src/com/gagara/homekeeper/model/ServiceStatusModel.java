package com.gagara.homekeeper.model;

import android.os.Bundle;

import com.gagara.homekeeper.service.BluetoothNbiService;

public class ServiceStatusModel implements Model {

    private BluetoothNbiService.State state = null;
    private String details = null;

    public ServiceStatusModel() {
    }

    public ServiceStatusModel(BluetoothNbiService.State state, String details) {
        super();
        this.state = state;
        this.details = details;
    }

    @Override
    public void saveState(Bundle bundle) {
        saveState(bundle, "");
    }

    @Override
    public void saveState(Bundle bundle, String prefix) {
        if (state != null) {
            bundle.putString(prefix + "service_status_state", state.toString());
        }
        bundle.putString(prefix + "service_status_details", details);
    }

    @Override
    public void restoreState(Bundle bundle) {
        restoreState(bundle, "");
    }

    @Override
    public void restoreState(Bundle bundle, String prefix) {
        String stateStr = bundle.getString(prefix + "service_status_state");
        if (stateStr != null) {
            state = BtCommunicationService.State.fromString(stateStr);
        }
        details = bundle.getString(prefix + "service_status_details");
    }

    @Override
    public boolean isInitialized() {
        return state != null;
    }

    public BluetoothNbiService.State getState() {
        return state;
    }

    public void setState(BluetoothNbiService.State state) {
        this.state = state;
    }

    public String getDetails() {
        return details;
    }

    public void setDetails(String details) {
        this.details = details;
    }

}

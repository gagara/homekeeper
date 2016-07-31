package com.gagara.homekeeper.model;

import android.os.Bundle;

import com.gagara.homekeeper.common.Mode;
import com.gagara.homekeeper.nbi.service.ServiceState;

public class ServiceStatusModel implements Model {

    private Mode mode = null;
    private ServiceState state = null;
    private String details = null;

    public ServiceStatusModel() {
    }

    public ServiceStatusModel(Mode mode, ServiceState state, String details) {
        super();
        this.mode = mode;
        this.state = state;
        this.details = details;
    }

    @Override
    public void saveState(Bundle bundle) {
        saveState(bundle, "");
    }

    @Override
    public void saveState(Bundle bundle, String prefix) {
        if (mode != null) {
            bundle.putString(prefix + "service_mode", mode.toString());
        }
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
        String modeStr = bundle.getString(prefix + "service_mode");
        if (modeStr != null) {
            mode = Mode.fromString(modeStr);
        }
        String stateStr = bundle.getString(prefix + "service_status_state");
        if (stateStr != null) {
            state = ServiceState.fromString(stateStr);
        }
        details = bundle.getString(prefix + "service_status_details");
    }

    @Override
    public boolean isInitialized() {
        return state != null;
    }

    public Mode getMode() {
        return mode;
    }

    public void setMode(Mode mode) {
        this.mode = mode;
    }

    public ServiceState getState() {
        return state;
    }

    public void setState(ServiceState state) {
        this.state = state;
    }

    public String getDetails() {
        return details;
    }

    public void setDetails(String details) {
        this.details = details;
    }
}

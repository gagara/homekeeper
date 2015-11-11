package com.gagara.homekeeper.model;

import android.os.Bundle;

import com.gagara.homekeeper.common.Mode;

public class ServiceTitleModel implements Model {

    private Mode mode = null;
    private String name = null;

    public ServiceTitleModel() {
    }

    public ServiceTitleModel(Mode mode, String name) {
        super();
        this.mode = mode;
        this.name = name;
    }

    @Override
    public void saveState(Bundle bundle) {
        saveState(bundle, "");
    }

    @Override
    public void saveState(Bundle bundle, String prefix) {
        if (mode != null) {
            bundle.putString(prefix + "service_state_mode", mode.toString());
        }
        bundle.putString(prefix + "service_state_name", name);
    }

    @Override
    public void restoreState(Bundle bundle) {
        restoreState(bundle, "");
    }

    @Override
    public void restoreState(Bundle bundle, String prefix) {
        String modeStr = bundle.getString(prefix + "service_state_mode");
        if (modeStr != null) {
            mode = Mode.fromString(modeStr);
        }
        name = bundle.getString(prefix + "service_state_name");
    }

    @Override
    public boolean isInitialized() {
        return mode != null;
    }

    public Mode getMode() {
        return mode;
    }

    public void setMode(Mode mode) {
        this.mode = mode;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }
}

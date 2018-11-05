package com.gagara.homekeeper.model;

import android.os.Bundle;

public class ServiceTitleModel implements Model {

    private String name = null;

    public ServiceTitleModel() {
    }

    public ServiceTitleModel(String name) {
        super();
        this.name = name;
    }

    @Override
    public void saveState(Bundle bundle) {
        saveState(bundle, "");
    }

    @Override
    public void saveState(Bundle bundle, String prefix) {
        bundle.putString(prefix + "service_state_name", name);
    }

    @Override
    public void restoreState(Bundle bundle) {
        restoreState(bundle, "");
    }

    @Override
    public void restoreState(Bundle bundle, String prefix) {
        name = bundle.getString(prefix + "service_state_name");
    }

    @Override
    public boolean isInitialized() {
        return true;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }
}

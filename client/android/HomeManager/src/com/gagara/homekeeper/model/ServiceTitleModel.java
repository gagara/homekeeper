package com.gagara.homekeeper.model;

import android.os.Bundle;

public class ServiceTitleModel implements Model {

    private String id = null;

    public ServiceTitleModel() {
    }

    public ServiceTitleModel(String id) {
        super();
        this.id = id;
    }

    @Override
    public void saveState(Bundle bundle) {
        saveState(bundle, "");
    }

    @Override
    public void saveState(Bundle bundle, String prefix) {
        bundle.putString(prefix + "selected_gw_id", id);
    }

    @Override
    public void restoreState(Bundle bundle) {
        restoreState(bundle, "");
    }

    @Override
    public void restoreState(Bundle bundle, String prefix) {
        id = bundle.getString(prefix + "selected_gw_id");
    }

    @Override
    public boolean isInitialized() {
        return true;
    }

    public String getId() {
        return id;
    }

    public void setId(String id) {
        this.id = id;
    }
}

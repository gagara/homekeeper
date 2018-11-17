package com.gagara.homekeeper.ui.viewmodel;

import java.util.ArrayList;
import java.util.Map;

import android.content.res.Resources;
import android.widget.ArrayAdapter;
import android.widget.Spinner;

import com.gagara.homekeeper.activity.Main;
import com.gagara.homekeeper.common.Gateway;
import com.gagara.homekeeper.model.ServiceTitleModel;
import com.gagara.homekeeper.utils.HomeKeeperConfig;

public class ServiceTitleModelView extends AbstractInfoModelView<ServiceTitleModel> implements
        ModelView<ServiceTitleModel> {

    public ServiceTitleModelView() {
        this.model = new ServiceTitleModel();
    }

    public ServiceTitleModelView(Spinner view, Resources resources) {
        this.model = new ServiceTitleModel();
        this.view = view;
        this.resources = resources;
    }

    @Override
    public void render() {
        Spinner view = (Spinner) this.view;
        Map<String, Gateway> gateways = HomeKeeperConfig.getAllNbiGateways(Main.getAppContext());
        if (gateways.size() > 0) {
            // update list
            ArrayList<String> values = new ArrayList<String>(gateways.size());
            for (Gateway gw : gateways.values()) {
                values.add(gw.getHost() + ":" + gw.getPort());
            }
            ArrayAdapter<String> adapter = new ArrayAdapter<String>(Main.getAppContext(),
                    android.R.layout.simple_spinner_item, values);
            view.setAdapter(adapter);
            if (model.getId() != null) {
                // select item
                String[] ids = gateways.keySet().toArray(new String[] {});
                for (int i = 0; i < gateways.size(); i++) {
                    if (ids[i].equals(model.getId())) {
                        view.setSelection(i);
                        break;
                    }
                }
            } else {
                // nothing selected
            }
        } else {
            // no gateways configured
        }
    }

    @Override
    public ServiceTitleModel getModel() {
        return model;
    }
}

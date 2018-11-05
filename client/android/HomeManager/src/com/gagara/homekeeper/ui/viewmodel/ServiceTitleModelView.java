package com.gagara.homekeeper.ui.viewmodel;

import android.content.res.Resources;
import android.widget.TextView;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.ServiceTitleModel;

public class ServiceTitleModelView extends AbstractInfoModelView<ServiceTitleModel> implements
        ModelView<ServiceTitleModel> {

    public ServiceTitleModelView() {
        this.model = new ServiceTitleModel();
    }

    public ServiceTitleModelView(TextView view, Resources resources) {
        this.model = new ServiceTitleModel();
        this.view = view;
        this.resources = resources;
    }

    @Override
    public void render() {
        TextView view = (TextView) this.view;
        if (model.getName() != null) {
            view.setText(String.format(resources.getString(R.string.service_title),
                    model.getName() != null ? model.getName() : resources.getString(R.string.unknown_service_provider)));
        } else {
            view.setText(null);
        }
    }

    @Override
    public ServiceTitleModel getModel() {
        return model;
    }
}

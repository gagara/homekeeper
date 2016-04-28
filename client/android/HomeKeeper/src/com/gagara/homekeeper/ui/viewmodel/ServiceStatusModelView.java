package com.gagara.homekeeper.ui.viewmodel;

import android.content.res.Resources;
import android.widget.TextView;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.ServiceStatusModel;

public class ServiceStatusModelView extends AbstractInfoModelView implements ModelView {

    public ServiceStatusModelView() {
        this.model = new ServiceStatusModel();
    }

    public ServiceStatusModelView(TextView view, Resources resources) {
        this.model = new ServiceStatusModel();
        this.view = view;
        this.resources = resources;
    }

    @Override
    public void render() {
        ServiceStatusModel model = (ServiceStatusModel) this.model;
        TextView view = (TextView) this.view;
        StringBuilder statusStr = new StringBuilder();
        if (model.getState() != null) {
            statusStr.append(resources.getString(model.getState().toStringResource()));
            if (model.getDetails() != null) {
                statusStr
                        .append(String.format(resources.getString(R.string.service_status_details), model.getDetails()));
            }
            view.setText(statusStr);
        } else {
            view.setText(null);
        }
    }

    @Override
    public ServiceStatusModel getModel() {
        return (ServiceStatusModel) model;
    }
}

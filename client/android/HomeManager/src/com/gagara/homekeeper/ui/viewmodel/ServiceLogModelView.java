package com.gagara.homekeeper.ui.viewmodel;

import android.content.res.Resources;
import android.widget.TextView;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.ServiceLogModel;
import com.gagara.homekeeper.model.ServiceStatusModel;

public class ServiceLogModelView extends AbstractInfoModelView<ServiceLogModel> implements ModelView<ServiceLogModel> {

    public ServiceLogModelView() {
        this.model = new ServiceLogModel();
    }

    public ServiceLogModelView(TextView view, Resources resources) {
        this.model = new ServiceLogModel();
        this.view = view;
        this.resources = resources;
    }

    @Override
    public void render() {
        TextView view = (TextView) this.view;
        StringBuilder logStr = new StringBuilder();
        for (ServiceStatusModel status : model.getLogs()) {
            if (logStr.length() > 0) {
                logStr.append("\n");
            }
            if (status.getMode() != null) {
                logStr.append(String.format(resources.getString(R.string.service_status_mode), status.getMode()
                        .toString()));
            }
            if (status.getState() != null) {
                logStr.append(String.format(resources.getString(R.string.service_status_state),
                        resources.getString(status.getState().toStringResource())));
            }
            if (status.getDetails() != null) {
                logStr.append(String.format(resources.getString(R.string.service_status_details), status.getDetails()));
            }
        }
        if (logStr.length() > 0) {
            view.setText(logStr.toString());
        } else {
            view.setText(null);
        }
    }

    @Override
    public ServiceLogModel getModel() {
        return model;
    }
}

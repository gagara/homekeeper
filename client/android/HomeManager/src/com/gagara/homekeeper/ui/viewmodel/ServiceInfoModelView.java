package com.gagara.homekeeper.ui.viewmodel;

import android.content.res.Resources;
import android.text.format.DateFormat;
import android.widget.TextView;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.ServiceInfoModel;

public class ServiceInfoModelView extends AbstractInfoModelView<ServiceInfoModel> implements
        ModelView<ServiceInfoModel> {

    public ServiceInfoModelView() {
        this.model = new ServiceInfoModel();
    }

    public ServiceInfoModelView(TextView view, Resources resources) {
        this.model = new ServiceInfoModel();
        this.view = view;
        this.resources = resources;
    }

    @Override
    public void render() {
        TextView view = (TextView) this.view;
        if (model.getTimestamp() != null) {
            view.setText(String.format(resources.getString(R.string.service_last_update),
                    DateFormat.format("HH:mm:ss", model.getTimestamp())));
        } else {
            view.setText(String.format(resources.getString(R.string.service_last_update), "--:--:--"));
        }
    }
}

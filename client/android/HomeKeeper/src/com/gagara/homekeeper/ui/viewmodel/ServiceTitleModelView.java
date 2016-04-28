package com.gagara.homekeeper.ui.viewmodel;

import android.content.res.Resources;
import android.widget.TextView;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.ServiceTitleModel;

public class ServiceTitleModelView extends AbstractInfoModelView implements ModelView {

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
        ServiceTitleModel model = (ServiceTitleModel) this.model;
        TextView view = (TextView) this.view;
        if (model.getMode() != null && model.getName() != null) {
            view.setText(String.format(resources.getString(R.string.service_title), model.getMode().toString(),
                    model.getName() != null ? model.getName() : resources.getString(R.string.unknown_bt_dev)));
        } else {
            view.setText(null);
        }
    }

    @Override
    public ServiceTitleModel getModel() {
        return (ServiceTitleModel) model;
    }
}
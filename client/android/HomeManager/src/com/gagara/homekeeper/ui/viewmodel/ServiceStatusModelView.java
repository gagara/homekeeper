package com.gagara.homekeeper.ui.viewmodel;

import static com.gagara.homekeeper.common.Constants.MAX_STATUS_RECORDS_TO_SHOW;
import android.content.res.Resources;
import android.widget.TextView;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.ServiceStatusModel;

public class ServiceStatusModelView extends AbstractInfoModelView implements ModelView {

    private String buffer = "";

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
        if (model.getMode() != null) {
            statusStr.append(String.format(resources.getString(R.string.service_status_mode), model.getMode()
                    .toString()));
        }
        if (model.getState() != null) {
            statusStr.append(String.format(resources.getString(R.string.service_status_state),
                    resources.getString(model.getState().toStringResource())));
        }
        if (model.getDetails() != null) {
            statusStr.append(String.format(resources.getString(R.string.service_status_details), model.getDetails()));
        }
        append(statusStr.toString());
        view.setText(fetch());
    }

    @Override
    public ServiceStatusModel getModel() {
        return (ServiceStatusModel) model;
    }

    private void append(String line) {
        if (line.length() > 0) {
            if (buffer.length() > 0) {
                buffer += "\n";
            }
            buffer += line;
        }
    }

    private String fetch() {
        if (buffer.length() > 0) {
            String[] lines = buffer.split("\n");
            buffer = "";
            int j = 0;
            for (int i = lines.length; i > 0; i--) {
                if (j < MAX_STATUS_RECORDS_TO_SHOW) {
                    if (j > 0) {
                        buffer = "\n" + buffer;
                    }
                    buffer = lines[i - 1] + buffer;
                    j++;
                } else {
                    break;
                }
            }
            return buffer;
        } else {
            return null;
        }
    }
}

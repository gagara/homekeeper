package com.gagara.homekeeper.ui.viewmodel;

import static com.gagara.homekeeper.common.Constants.UNDEFINED_DATE;

import java.util.Date;

import android.text.format.DateFormat;
import android.widget.TextView;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.StateSensorModel;
import com.gagara.homekeeper.ui.view.ViewUtils;

public class StateSensorModelView extends SensorModelView<StateSensorModel> implements ModelView<StateSensorModel> {

    private static final String DATE_FORMAT = "HH:mm dd/MM";

    public StateSensorModelView(int id) {
        model = new StateSensorModel(id);
    }

    @Override
    public void render() {
        TextView valueView = (TextView) this.valueView;
        TextView detailsView = (TextView) this.detailsView;

        if (model.isInitialized()) {
            valueView.setText(model.getState() ? resources.getString(R.string.sensor_state_value_true) : resources
                    .getString(R.string.sensor_state_value_false));
        } else {
            valueView.setText(resources.getString(R.string.sensor_state_value_unknown));
        }

        StringBuilder detailsStr = new StringBuilder();
        if (!UNDEFINED_DATE.equals(model.getSwitchTimestamp())) {
            Date currDate = new Date();
            String elapsedMinsStr = ViewUtils.buildElapseTimeString(currDate, model.getSwitchTimestamp());
            detailsStr.append(String.format(resources.getString(R.string.sensor_details_time_template),
                    DateFormat.format(DATE_FORMAT, model.getSwitchTimestamp()), elapsedMinsStr));
        }
        detailsView.setText(detailsStr);
    }
}

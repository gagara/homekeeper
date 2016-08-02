package com.gagara.homekeeper.ui.viewmodel;

import static com.gagara.homekeeper.common.Constants.UNDEFINED_DATE;
import static com.gagara.homekeeper.common.Constants.UNDEFINED_SENSOR_VALUE;
import static com.gagara.homekeeper.common.Constants.UNKNOWN_SENSOR_VALUE;
import static com.gagara.homekeeper.ui.view.ViewUtils.getSensorSignResourceByType;

import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.util.Date;

import android.text.format.DateFormat;
import android.widget.TextView;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.SensorModel;
import com.gagara.homekeeper.ui.view.ViewUtils;

public class SensorModelView extends AbstractEntryModelView<SensorModel> implements ModelView<SensorModel> {

    private static final String DATE_FORMAT = "HH:mm dd/MM";

    public SensorModelView(int id) {
        model = new SensorModel(id);
    }

    @Override
    public void render() {
        TextView valueView = (TextView) this.valueView;
        TextView detailsView = (TextView) this.detailsView;

        NumberFormat f = new DecimalFormat("#0");

        if (model.isInitialized() && model.getValue() != UNDEFINED_SENSOR_VALUE
                && model.getValue() != UNKNOWN_SENSOR_VALUE) {
            valueView.setText(String.format(resources.getString(R.string.sensor_value_template),
                    f.format(model.getValue()), resources.getString(getSensorSignResourceByType(model.getType()))));
        } else {
            valueView.setText(String.format(resources.getString(R.string.sensor_value_template), "?",
                    resources.getString(getSensorSignResourceByType(model.getType()))));
        }

        StringBuilder detailsStr = new StringBuilder();
        if (model.isInitialized() && model.getPrevValue() != UNDEFINED_SENSOR_VALUE
                && model.getPrevValue() != UNKNOWN_SENSOR_VALUE) {
            double delta = model.getValue() - model.getPrevValue();
            int res;
            if (delta > 0) {
                res = R.string.sensor_details_inc_template;
            } else if (delta < 0) {
                res = R.string.sensor_details_dec_template;
            } else {
                res = R.string.sensor_details_equ_template;
            }
            detailsStr.append(String.format(resources.getString(res), f.format(Math.abs(delta))));
        }

        if (!UNDEFINED_DATE.equals(model.getTimestamp())) {
            Date currDate = new Date();
            String elapsedMinsStr = ViewUtils.buildElapseTimeString(currDate, model.getTimestamp());
            if (detailsStr.length() > 0) {
                detailsStr.append(" ");
            }
            detailsStr.append(String.format(resources.getString(R.string.sensor_details_time_template),
                    DateFormat.format(DATE_FORMAT, model.getTimestamp()), elapsedMinsStr));
        }
        detailsView.setText(detailsStr);
    }
}

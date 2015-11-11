package com.gagara.homekeeper.ui.viewmodel;

import static com.gagara.homekeeper.common.Constants.UNDEFINED_SENSOR_VALUE;

import java.text.DecimalFormat;
import java.text.NumberFormat;

import android.widget.TextView;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.SensorModel;

public class SensorModelView extends AbstractEntryModelView implements ModelView {

    public SensorModelView(int id) {
        model = new SensorModel(id);
    }

    @Override
    public void render() {
        SensorModel model = (SensorModel) this.model;
        TextView valueView = (TextView) this.valueView;
        TextView detailsView = (TextView) this.detailsView;

        NumberFormat f = new DecimalFormat("#0");

        if (model.isInitialized() && model.getValue() != UNDEFINED_SENSOR_VALUE) {
            valueView.setText(String.format(resources.getString(R.string.sensor_value_template),
                    f.format(model.getValue())));
        } else {
            valueView.setText(String.format(resources.getString(R.string.sensor_value_template),
                    "?"));
        }

        if (model.isInitialized() && model.getPrevValue() != UNDEFINED_SENSOR_VALUE) {
            double delta = model.getValue() - model.getPrevValue();
            int res;
            if (delta > 0) {
                res = R.string.sensor_details_inc_template;
            } else if (delta < 0) {
                res = R.string.sensor_details_dec_template;
            } else {
                res = R.string.sensor_details_equ_template;
            }
            detailsView.setText(String.format(resources.getString(res), f.format(Math.abs(delta))));
        } else {
            detailsView.setText(null);
        }
    }

    @Override
    public SensorModel getModel() {
        return (SensorModel) model;
    }
}

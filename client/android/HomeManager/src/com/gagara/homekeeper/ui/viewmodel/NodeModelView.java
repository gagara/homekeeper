package com.gagara.homekeeper.ui.viewmodel;

import static com.gagara.homekeeper.common.Constants.UNDEFINED_DATE;
import static com.gagara.homekeeper.common.Constants.UNDEFINED_SENSOR_VALUE;
import static com.gagara.homekeeper.common.Constants.UNKNOWN_SENSOR_VALUE;
import static com.gagara.homekeeper.ui.view.ViewUtils.getSensorSignResourceByType;

import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.util.Date;

import android.text.format.DateFormat;
import android.view.View;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.NodeModel;
import com.gagara.homekeeper.model.SensorModel;
import com.gagara.homekeeper.ui.view.ViewUtils;

public class NodeModelView extends AbstractEntryModelView<NodeModel> implements ModelView<NodeModel> {

    private static final String DATE_FORMAT = "HH:mm dd/MM";

    protected View configView;

    public NodeModelView(int id) {
        model = new NodeModel(id);
    }

    public View getConfigView() {
        return configView;
    }

    public void setConfigView(View configView) {
        this.configView = configView;
    }

    @Override
    public void render() {
        ToggleButton valueView = (ToggleButton) this.valueView;
        TextView detailsView = (TextView) this.detailsView;
        TextView configView = (TextView) this.configView;
        NumberFormat f = new DecimalFormat("#0");
        Date currDate = new Date();
        String elapsedMinsStr = "0";
        String remainingMinsStr = "0";

        // details
        if (model.isInitialized()) {
            valueView.setChecked(model.getState());
            valueView.setEnabled(true);
            if (!UNDEFINED_DATE.equals(model.getSwitchTimestamp())) {
                elapsedMinsStr = ViewUtils.buildElapseTimeString(currDate, model.getSwitchTimestamp());
            }

            if (!UNDEFINED_DATE.equals(model.getForcedModeTimestamp())) {
                remainingMinsStr = ViewUtils.buildElapseTimeString(model.getForcedModeTimestamp(), currDate);
            }

            StringBuilder detailsStr = new StringBuilder();
            if (model.isForcedMode()) {
                detailsStr.append(resources.getString(R.string.node_details_forced_flag));
                detailsStr.append(model.getState() ? resources.getString(R.string.node_details_switch_on) : resources
                        .getString(R.string.node_details_switch_off));

                detailsStr.append(resources.getString(R.string.node_details_elapsed_time_flag));
                if (!UNDEFINED_DATE.equals(model.getSwitchTimestamp())) {
                    detailsStr.append(String.format(resources.getString(R.string.node_details_time_template),
                            DateFormat.format(DATE_FORMAT, model.getSwitchTimestamp()), elapsedMinsStr));
                } else {
                    detailsStr.append(resources.getString(R.string.node_details_unknown_time));
                }

                detailsStr.append(resources.getString(R.string.node_details_remainig_time_flag));
                if (!UNDEFINED_DATE.equals(model.getForcedModeTimestamp())) {
                    detailsStr.append(String.format(resources.getString(R.string.node_details_time_template),
                            DateFormat.format(DATE_FORMAT, model.getForcedModeTimestamp()), remainingMinsStr));
                } else {
                    detailsStr.append(resources.getString(R.string.node_details_permanent_time));
                }
            } else {
                detailsStr.append(model.getState() ? resources.getString(R.string.node_details_switch_on) : resources
                        .getString(R.string.node_details_switch_off));

                detailsStr.append(resources.getString(R.string.node_details_elapsed_time_flag));
                if (!UNDEFINED_DATE.equals(model.getSwitchTimestamp())) {
                    detailsStr.append(String.format(resources.getString(R.string.node_details_time_template),
                            DateFormat.format(DATE_FORMAT, model.getSwitchTimestamp()), elapsedMinsStr));
                } else {
                    detailsStr.append(resources.getString(R.string.node_details_unknown_time));
                }

                if (model.getSensors().size() > 0) {
                    detailsStr.append(resources.getString(R.string.node_details_sensors_template));
                    for (int i = 0; i < model.getSensors().size(); i++) {
                        SensorModel s = model.getSensors().valueAt(i);
                        String value = "?";
                        if (s.getValue() != UNDEFINED_SENSOR_VALUE && s.getValue() != UNKNOWN_SENSOR_VALUE) {
                            value = f.format(s.getValue());
                        }
                        String sensorName = "id_" + s.getId();
                        if (ViewUtils.validSensor(s)) {
                            sensorName = resources.getString(TopModelView.SENSORS.get(s.getId()));
                        }
                        detailsStr.append(String.format(resources.getString(R.string.node_details_sensor_template),
                                sensorName, value, resources.getString(getSensorSignResourceByType(s.getType()))));
                    }
                }

            }
            detailsView.setText(detailsStr.toString());
        } else {
            valueView.setChecked(true);
            valueView.setEnabled(false);
            detailsView.setText(R.string.node_details_unknown);
        }

        // config
        StringBuilder configStr = new StringBuilder();
        if (model.getSensorsThresholds().size() > 0) {
            for (int i = 0; i < model.getSensorsThresholds().size(); i++) {
                SensorModel s = model.getSensorsThresholds().valueAt(i);
                String value = "?";
                if (s.getValue() != UNDEFINED_SENSOR_VALUE && s.getValue() != UNKNOWN_SENSOR_VALUE) {
                    value = f.format(s.getValue());
                }
                String sensorName = "id_" + s.getId();
                if (ViewUtils.validSensor(s)) {
                    sensorName = resources.getString(TopModelView.SENSORS_THRESHOLDS.get(s.getId()));
                }
                configStr.append(String.format(resources.getString(R.string.node_config_sensors_template), String
                        .format(resources.getString(R.string.node_config_sensor_template), sensorName, value,
                                resources.getString(getSensorSignResourceByType(s.getType())))));
            }
        }
        configView.setText(configStr.toString());
    }
}

package com.gagara.homekeeper.ui.viewmodel;

import static com.gagara.homekeeper.common.Constants.UNDEFINED_DATE;

import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.util.Date;

import android.text.format.DateFormat;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.NodeModel;
import com.gagara.homekeeper.model.SensorModel;

public class NodeModelView extends AbstractEntryModelView implements ModelView {

    private static final String DATE_FORMAT = "HH:mm dd/MM";

    public NodeModelView(int id) {
        model = new NodeModel(id);
    }

    @Override
    public void render() {
        NodeModel model = (NodeModel) this.model;
        ToggleButton valueView = (ToggleButton) this.valueView;
        TextView detailsView = (TextView) this.detailsView;
        NumberFormat f = new DecimalFormat("#0");
        Date currDate = new Date();
        String elapsedMinsStr = "0";
        String remainingMinsStr = "0";

        if (model.isInitialized()) {
            valueView.setChecked(model.getState());
            valueView.setEnabled(true);
            if (!UNDEFINED_DATE.equals(model.getSwitchTimestamp())) {
                elapsedMinsStr = buildElapseTimeString(currDate, model.getSwitchTimestamp());
            }

            if (!UNDEFINED_DATE.equals(model.getForcedModeTimestamp())) {
                remainingMinsStr = buildElapseTimeString(model.getForcedModeTimestamp(), currDate);
            }

            StringBuilder detailsStr = new StringBuilder();
            if (model.isForcedMode()) {
                detailsStr.append(resources.getString(R.string.node_details_forced_flag));
                detailsStr.append(model.getState() ? resources
                        .getString(R.string.node_details_switch_on) : resources
                        .getString(R.string.node_details_switch_off));

                detailsStr.append(resources.getString(R.string.node_details_elapsed_time_flag));
                if (!UNDEFINED_DATE.equals(model.getSwitchTimestamp())) {
                    detailsStr.append(String.format(
                            resources.getString(R.string.node_details_time_template),
                            DateFormat.format(DATE_FORMAT, model.getSwitchTimestamp()),
                            elapsedMinsStr));
                } else {
                    detailsStr.append(resources.getString(R.string.node_details_unknown_time));
                }

                detailsStr.append(resources.getString(R.string.node_details_remainig_time_flag));
                if (!UNDEFINED_DATE.equals(model.getForcedModeTimestamp())) {
                    detailsStr.append(String.format(
                            resources.getString(R.string.node_details_time_template),
                            DateFormat.format(DATE_FORMAT, model.getForcedModeTimestamp()),
                            remainingMinsStr));
                } else {
                    detailsStr.append(resources.getString(R.string.node_details_permanent_time));
                }
            } else {
                detailsStr.append(model.getState() ? resources
                        .getString(R.string.node_details_switch_on) : resources
                        .getString(R.string.node_details_switch_off));

                detailsStr.append(resources.getString(R.string.node_details_elapsed_time_flag));
                if (!UNDEFINED_DATE.equals(model.getSwitchTimestamp())) {
                    detailsStr.append(String.format(
                            resources.getString(R.string.node_details_time_template),
                            DateFormat.format(DATE_FORMAT, model.getSwitchTimestamp()),
                            elapsedMinsStr));
                } else {
                    detailsStr.append(resources.getString(R.string.node_details_unknown_time));
                }

                if (model.getSensors().size() > 0) {
                    detailsStr.append(resources.getString(R.string.node_details_sensors_template));
                    for (int i = 0; i < model.getSensors().size(); i++) {
                        SensorModel s = model.getSensors().valueAt(i);
                        detailsStr.append(String.format(resources
                                .getString(R.string.node_details_sensor_template), resources
                                .getString(TopModelView.SENSORS_NAME_VIEW_MAP.get(s.getId())), f
                                .format(s.getValue())));
                    }
                }

            }
            detailsView.setText(detailsStr.toString());
        } else {
            valueView.setChecked(true);
            valueView.setEnabled(false);
            detailsView.setText(R.string.node_details_unknown);
        }
    }

    @Override
    public NodeModel getModel() {
        return (NodeModel) model;
    }

    private String buildElapseTimeString(Date hi, Date lo) {
        String result;
        double delta = hi.getTime() - lo.getTime();
        if (delta > 0) {
            delta = Math.round(delta / (1000 * 60));
            if (delta == 0) {
                result = "<1";
            } else {
                result = "" + Double.valueOf(delta).intValue();
            }
        } else {
            result = "0";
        }
        return result;
    }
}

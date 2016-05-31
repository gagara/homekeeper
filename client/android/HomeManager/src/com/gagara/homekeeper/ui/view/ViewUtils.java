package com.gagara.homekeeper.ui.view;

import static com.gagara.homekeeper.common.ControllerConfig.NODE_BOILER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_CIRCULATION_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_FLOOR_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_HEATING_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_HOTWATER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_SB_HEATER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.NODE_SUPPLY_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_BOILER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_MIX_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_REVERSE_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_ROOM1_HUM_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_ROOM1_TEMP_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_SB_HEATER_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_SUPPLY_ID;
import static com.gagara.homekeeper.common.ControllerConfig.SENSOR_TANK_ID;
import android.content.res.Resources;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.NodeModel;
import com.gagara.homekeeper.model.SensorModel;
import com.gagara.homekeeper.model.SensorModel.SensorType;

public class ViewUtils {

    private static Resources resources;

    public static String getSensorSignByType(SensorType type) {
        if (SensorType.TEMPERATURE == type) {
            return resources.getString(R.string.temperature_sign);
        } else if (SensorType.HUMIDITY == type) {
            return resources.getString(R.string.humidity_sign);
        } else {
            return "?";
        }
    }

    public static boolean validSensor(SensorModel sensor) {
        int id = sensor.getId();
        return id == SENSOR_SUPPLY_ID || id == SENSOR_REVERSE_ID || id == SENSOR_TANK_ID || id == SENSOR_BOILER_ID
                || id == SENSOR_MIX_ID || id == SENSOR_SB_HEATER_ID || id == SENSOR_ROOM1_TEMP_ID
                || id == SENSOR_ROOM1_HUM_ID;
    }

    public static boolean validNode(NodeModel node) {
        int id = node.getId();
        return id == NODE_SUPPLY_ID || id == NODE_HEATING_ID || id == NODE_FLOOR_ID || id == NODE_HOTWATER_ID
                || id == NODE_CIRCULATION_ID || id == NODE_BOILER_ID || id == NODE_SB_HEATER_ID;
    }
}

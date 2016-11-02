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

import java.util.Date;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.model.NodeModel;
import com.gagara.homekeeper.model.SensorModel;
import com.gagara.homekeeper.model.SensorModel.SensorType;

public class ViewUtils {

    private static final int SENSOR_DETAILS_VIEW_MOD = 100;
    private static final int SENSOR_VALUE_VIEW_MOD = 1000;

    private static final int NODE_DETAILS_VIEW_MOD = 100;
    private static final int NODE_VALUE_VIEW_MOD = 1000;
    private static final int NODE_CONFIG_VIEW_MOD = 10000;
    private static final int NODE_INFO_VIEW_MOD = 100000;

    public static String buildElapseTimeString(Date hi, Date lo) {
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

    public static int getSensorSignResourceByType(SensorType type) {
        if (SensorType.TEMPERATURE == type) {
            return R.string.temperature_sign;
        } else if (SensorType.HUMIDITY == type) {
            return R.string.humidity_sign;
        } else {
            return R.string.unknown_sign;
        }
    }

    public static boolean validSensor(SensorModel sensor) {
        int id = sensor != null ? sensor.getId() : -1;
        return id == SENSOR_SUPPLY_ID || id == SENSOR_REVERSE_ID || id == SENSOR_TANK_ID || id == SENSOR_BOILER_ID
                || id == SENSOR_MIX_ID || id == SENSOR_SB_HEATER_ID || id == SENSOR_ROOM1_TEMP_ID
                || id == SENSOR_ROOM1_HUM_ID;
    }

    public static boolean validNode(NodeModel node) {
        int id = node != null ? node.getId() : -1;
        return id == NODE_SUPPLY_ID || id == NODE_HEATING_ID || id == NODE_FLOOR_ID || id == NODE_HOTWATER_ID
                || id == NODE_CIRCULATION_ID || id == NODE_BOILER_ID || id == NODE_SB_HEATER_ID;
    }

    public static int getSensorDetailsViewId(int id) {
        return id * SENSOR_DETAILS_VIEW_MOD;
    }

    public static int getSensorValueViewId(int id) {
        return id * SENSOR_VALUE_VIEW_MOD;
    }

    public static int getNodeDetailsViewId(int id) {
        return id * NODE_DETAILS_VIEW_MOD;
    }

    public static int getNodeValueViewId(int id) {
        return id * NODE_VALUE_VIEW_MOD;
    }

    public static int getNodeConfigViewId(int id) {
        return id * NODE_CONFIG_VIEW_MOD;
    }

    public static int getNodeInfoViewId(int id) {
        return id * NODE_INFO_VIEW_MOD;
    }

    public static int getNodeIdByValueViewId(int viewId) {
        return viewId / NODE_VALUE_VIEW_MOD;
    }

    public static int getNodeIdByInfoViewId(int viewId) {
        return viewId / NODE_INFO_VIEW_MOD;
    }
}

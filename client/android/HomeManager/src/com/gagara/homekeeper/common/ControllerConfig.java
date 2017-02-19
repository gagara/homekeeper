package com.gagara.homekeeper.common;

public class ControllerConfig {

    private ControllerConfig() {
    }

    public enum MessageType {
        CURRENT_STATUS_REPORT("csr"), NODE_STATE_CHANGED("nsc"), CLOCK_SYNC("cls"), FAST_CLOCK_SYNC("fcs"), LOG("log"), CONFIGURATION(
                "cfg");

        private String code;

        private MessageType(String code) {
            this.code = code;
        }

        public String code() {
            return this.code;
        }

        public static MessageType forCode(String code) {
            for (MessageType m : values()) {
                if (m.code.equalsIgnoreCase(code)) {
                    return m;
                }
            }
            return null;
        }
    }

    public static final int SENSOR_SUPPLY_ID = 54;
    public static final int SENSOR_REVERSE_ID = 55;
    public static final int SENSOR_TANK_ID = 56;
    public static final int SENSOR_BOILER_ID = 57;
    public static final int SENSOR_MIX_ID = 58;
    public static final int SENSOR_SB_HEATER_ID = 59;
    public static final int SENSOR_ROOM1_TEMP_ID = 74;
    public static final int SENSOR_ROOM1_HUM_ID = 75;

    public static final int SENSOR_TH_ROOM1_SB_HEATER_ID = 201;
    public static final int SENSOR_TH_ROOM1_PRIMARY_HEATER_ID = 202;

    public static final int NODE_SUPPLY_ID = 22;
    public static final int NODE_HEATING_ID = 24;
    public static final int NODE_FLOOR_ID = 26;
    public static final int NODE_HOTWATER_ID = 28;
    public static final int NODE_CIRCULATION_ID = 30;
    public static final int NODE_BOILER_ID = 32;
    public static final int NODE_SB_HEATER_ID = 34;

    public static final String MSG_TYPE_KEY = "m";
    public static final String ID_KEY = "id";
    public static final String STATE_KEY = "ns";
    public static final String FORCE_FLAG_KEY = "ff";
    public static final String TIMESTAMP_KEY = "ts";
    public static final String FORCE_TIMESTAMP_KEY = "ft";
    public static final String OVERFLOW_COUNT_KEY = "oc";
    public static final String NODE_KEY = "n";
    public static final String SENSOR_KEY = "s";
    public static final String VALUE_KEY = "v";

    public static final long MAX_INACTIVE_PERIOD_SEC = 90;
}

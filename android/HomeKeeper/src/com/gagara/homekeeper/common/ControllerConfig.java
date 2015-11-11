package com.gagara.homekeeper.common;

public class ControllerConfig {

    private ControllerConfig() {
    }

    public enum MessageType {
        CURRENT_STATUS_REPORT("csr"), NODE_STATE_CHANGED("nsc"), CLOCK_SYNC("cls");

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

    public static final int SENSOR_SUPPLY_ID = 14;
    public static final int SENSOR_REVERSE_ID = 15;
    public static final int SENSOR_TANK_ID = 16;
    public static final int SENSOR_BOILER_ID = 17;

    public static final int NODE_SUPPLY_ID = 6;
    public static final int NODE_HEATING_ID = 7;
    public static final int NODE_FLOOR_ID = 8;
    public static final int NODE_HOTWATER_ID = 9;
    public static final int NODE_CIRCULATION_ID = 10;
    public static final int NODE_BOILER_ID = 11;

    public static final String MSG_TYPE_KEY = "m";
    public static final String ID_KEY = "id";
    public static final String STATE_KEY = "ns";
    public static final String FORCE_FLAG_KEY = "ff";
    public static final String TIMESTAMP_KEY = "ts";
    public static final String FORCE_TIMESTAMP_KEY = "ft";
    public static final String OVERFLOW_COUNT_KEY = "oc";
    public static final String NODES_KEY = "n";
    public static final String SENSORS_KEY = "s";
    public static final String VALUE_KEY = "v";

}

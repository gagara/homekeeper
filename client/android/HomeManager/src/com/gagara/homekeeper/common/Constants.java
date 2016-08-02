package com.gagara.homekeeper.common;

import java.util.Date;

public class Constants {

    private Constants() {
    }

    public static final int UNDEFINED_SENSOR_VALUE = Integer.MAX_VALUE;
    public static final int UNKNOWN_SENSOR_VALUE = -127;
    public static final Date UNDEFINED_DATE = new Date(0);
    public static final int UNDEFINED_ID = -1;

    public static final int DEFAULT_REMOTE_CONTROL_PULL_INTERVAL = 10;
    public static final int DEFAULT_SLAVE_REFRESH_INTERVAL = 30;

    public static final int REQUEST_ENABLE_BT = 306;
    public static final int REQUEST_ENABLE_NETWORK = 506;
    public static final int CONTROLLER_SERVICE_ONGOING_NOTIFICATION_ID = 1006;
    public static final int CONTROLLER_SERVICE_PENDING_INTENT_ID = 2206;
    
    public static final int DEFAULT_SWITCH_OFF_PERIOD_SEC = 0;     // forever
    public static final int DEFAULT_SWITCH_ON_PERIOD_SEC = 330;    // 5 minutes

    public static final int MAX_STATUS_RECORDS_TO_SHOW = 2;

    public static final String CONTROLLER_DATA_TRANSFER_ACTION = "HOME_KEEPER_CONTROLLER_DATA_TRANSFER";
    public static final String CONTROLLER_CONTROL_COMMAND_ACTION = "HOME_KEEPER_CONTROLLER_CONTROL_COMMAND";
    public static final String SERVICE_STATUS_CHANGE_ACTION = "HOME_KEEPER_SERVICE_STATUS_CHANGE_COMMAND";
    public static final String SERVICE_TITLE_CHANGE_ACTION = "HOME_KEEPER_SERVICE_TITLE_CHANGE_COMMAND";

    public static final String SWITCH_NODE_DIALOG_TAG = "SWITCH_NODE_STATE_DIALOG";

    public static final String DATA_KEY = "DATA";
    public static final String COMMAND_KEY = "COMMAND";
    public static final String SERVICE_STATUS_KEY = "STATUS";
    public static final String SERVICE_STATUS_DETAILS_KEY = "STATUS_DETAILS";
    public static final String SERVICE_TITLE_KEY = "TITLE";

    public static final String CFG_MODE = "mode";
    public static final String CFG_BT_DEV = "bt_dev";
    public static final String CFG_PROXY_HOST = "proxy_host";
    public static final String CFG_PROXY_PORT = "proxy_port";
    public static final String CFG_PROXY_USER = "proxy_user";
    public static final String CFG_PROXY_PASSWORD = "proxy_password";
    public static final String CFG_PROXY_PULL_PERIOD = "proxy_pull_period";

    public static final String MESSAGE_KEY = "message";
    public static final String TIMESTAMP_KEY = "@timestamp";
    
}

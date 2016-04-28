package com.gagara.homekeeper.common;

import java.util.Date;

public class Constants {

    private Constants() {
    }

    public static final int UNDEFINED_SENSOR_VALUE = Integer.MAX_VALUE;
    public static final Date UNDEFINED_DATE = new Date(0);
    public static final int UNDEFINED_ID = -1;

    public static final int DEFAULT_REMOTE_CONTROL_PULL_INTERVAL = 10;
    public static final int DEFAULT_SLAVE_REFRESH_INTERVAL = 30;

    public static final int REQUEST_ENABLE_BT = 306;
    public static final int CONTROLLER_SERVICE_ONGOING_NOTIFICATION_ID = 1006;
    public static final int CONTROLLER_SERVICE_PENDING_INTENT_ID = 2206;
    
    public static final int DEFAULT_SWITCH_OFF_PERIOD_SEC = 0;     // forever
    public static final int DEFAULT_SWITCH_ON_PERIOD_SEC = 330;    // 5 minutes

    public static final String CONTROLLER_DATA_TRANSFER_ACTION = "HOME_KEEPER_CONTROLLER_DATA_TRANSFER";
    public static final String CONTROLLER_CONTROL_COMMAND_ACTION = "HOME_KEEPER_CONTROLLER_CONTROL_COMMAND";
    public static final String BT_SERVICE_STATUS_ACTION = "HOME_KEEPER_BT_SERVICE_STATUS_COMMAND";

    public static final String SWITCH_NODE_DIALOG_TAG = "SWITCH_NODE_STATE_DIALOG";

    public static final String DATA_KEY = "DATA";
    public static final String COMMAND_KEY = "COMMAND";
    public static final String SERVICE_STATUS_KEY = "STATUS";
    public static final String SERVICE_STATUS_DETAILS_KEY = "STATUS_DETAILS";

    public static final String CFG_MODE = "mode";
    public static final String CFG_BT_DEV = "bt_dev";
    public static final String CFG_DATA_PUBLISHING = "data_publishing";
    public static final String CFG_REMOTE_CONTROL = "remote_control";
    public static final String CFG_REMOTE_CONTROL_PULL_INTERVAL = "remote_control_pull_interval";
    public static final String CFG_SLAVE_REFRESH_INTERVAL = "slave_refresh_interval";
    public static final String CFG_REMOTE_SERVICE_ENDPOINT = "remote_endpoint";

    public static final String PREFERENCE_GENERAL_CATEGORY_KEY = "pref_general_category";
    public static final String PREFERENCE_MASTER_CATEGORY_KEY = "pref_master_category";
}
package com.gagara.homekeeper.utils;

import static com.gagara.homekeeper.common.Constants.CFG_BT_DEV;
import static com.gagara.homekeeper.common.Constants.CFG_DATA_PUBLISHING;
import static com.gagara.homekeeper.common.Constants.CFG_MODE;
import static com.gagara.homekeeper.common.Constants.CFG_REMOTE_CONTROL;
import static com.gagara.homekeeper.common.Constants.CFG_REMOTE_CONTROL_PULL_INTERVAL;
import static com.gagara.homekeeper.common.Constants.CFG_REMOTE_SERVICE_ENDPOINT;
import static com.gagara.homekeeper.common.Constants.CFG_SLAVE_REFRESH_INTERVAL;
import static com.gagara.homekeeper.common.Constants.DEFAULT_REMOTE_CONTROL_PULL_INTERVAL;
import static com.gagara.homekeeper.common.Constants.DEFAULT_SLAVE_REFRESH_INTERVAL;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.preference.PreferenceManager;

import com.gagara.homekeeper.common.Mode;

public class HomeKeeperConfig {

    private HomeKeeperConfig() {
    }

    public static Mode getMode(Context ctx) {
        return getMode(ctx, Mode.MASTER);
    }

    public static Mode getMode(Context ctx, Mode defValue) {
        return Mode.fromString(PreferenceManager.getDefaultSharedPreferences(ctx).getString(CFG_MODE,
                defValue != null ? defValue.toString() : null));
    }

    public static BluetoothDevice getBluetoothDevice(Context ctx) {
        return getBluetoothDevice(ctx, null);
    }

    public static BluetoothDevice getBluetoothDevice(Context ctx, BluetoothDevice defValue) {
        BluetoothDevice dev = null;
        String devAddr = PreferenceManager.getDefaultSharedPreferences(ctx).getString(CFG_BT_DEV,
                defValue != null ? defValue.getName() : null);
        if (devAddr != null) {
            BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
            if (bluetoothAdapter != null) {
                for (BluetoothDevice d : bluetoothAdapter.getBondedDevices()) {
                    if (d.getAddress().equals(devAddr)) {
                        dev = d;
                        break;
                    }
                }
            }
        }
        return dev;
    }

    public static boolean isDataPublishingEnabled(Context ctx) {
        return isDataPublishingEnabled(ctx, false);
    }

    public static boolean isDataPublishingEnabled(Context ctx, boolean defValue) {
        return PreferenceManager.getDefaultSharedPreferences(ctx).getBoolean(CFG_DATA_PUBLISHING, defValue);
    }

    public static boolean isRemoteControlEnabled(Context ctx) {
        return isRemoteControlEnabled(ctx, false);
    }

    public static boolean isRemoteControlEnabled(Context ctx, boolean defValue) {
        return PreferenceManager.getDefaultSharedPreferences(ctx).getBoolean(CFG_REMOTE_CONTROL, defValue);
    }

    public static int getRemoteControlPullInterval(Context ctx) {
        return getRemoteControlPullInterval(ctx, DEFAULT_REMOTE_CONTROL_PULL_INTERVAL);
    }

    public static int getRemoteControlPullInterval(Context ctx, int defValue) {
        return PreferenceManager.getDefaultSharedPreferences(ctx).getInt(CFG_REMOTE_CONTROL_PULL_INTERVAL, defValue);
    }

    public static int getSlaveRefreshInterval(Context ctx) {
        return getSlaveRefreshInterval(ctx, DEFAULT_SLAVE_REFRESH_INTERVAL);
    }

    public static int getSlaveRefreshInterval(Context ctx, int defValue) {
        return PreferenceManager.getDefaultSharedPreferences(ctx).getInt(CFG_SLAVE_REFRESH_INTERVAL, defValue);
    }

    public static String getRemoteEndpoint(Context ctx) {
        return getRemoteEndpoint(ctx, "");
    }

    public static String getRemoteEndpoint(Context ctx, String defValue) {
        return PreferenceManager.getDefaultSharedPreferences(ctx).getString(CFG_REMOTE_SERVICE_ENDPOINT, defValue);
    }
}

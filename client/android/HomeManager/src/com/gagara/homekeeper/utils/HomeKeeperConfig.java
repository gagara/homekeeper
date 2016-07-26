package com.gagara.homekeeper.utils;

import static com.gagara.homekeeper.common.Constants.CFG_BT_DEV;
import static com.gagara.homekeeper.common.Constants.CFG_MODE;
import static com.gagara.homekeeper.common.Constants.CFG_PROXY_HOST;
import static com.gagara.homekeeper.common.Constants.CFG_PROXY_PORT;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.preference.PreferenceManager;

import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.Mode;
import com.gagara.homekeeper.common.Proxy;

public class HomeKeeperConfig {

    private HomeKeeperConfig() {
    }

    public static Mode getMode(Context ctx) {
        return getMode(ctx, Mode.DIRECT);
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

    public static Proxy getNbiProxy(Context ctx) {
        return getNbiProxy(ctx, null);
    }

    public static Proxy getNbiProxy(Context ctx, Proxy defValue) {
        return new Proxy(PreferenceManager.getDefaultSharedPreferences(ctx).getString(CFG_PROXY_HOST,
                defValue != null ? defValue.getHost() : null), PreferenceManager.getDefaultSharedPreferences(ctx)
                .getInt(CFG_PROXY_PORT, defValue != null ? defValue.getPort() : 0), PreferenceManager
                .getDefaultSharedPreferences(ctx).getString(Constants.CFG_PROXY_USER,
                        defValue != null ? defValue.getUsername() : null), PreferenceManager
                .getDefaultSharedPreferences(ctx).getString(Constants.CFG_PROXY_PASSWORD,
                        defValue != null ? defValue.getPassword() : null), PreferenceManager
                .getDefaultSharedPreferences(ctx).getInt(Constants.CFG_PROXY_PULL_PERIOD,
                        defValue != null ? defValue.getPullPeriod() : 0));
    }

}

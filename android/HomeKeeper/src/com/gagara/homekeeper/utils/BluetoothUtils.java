package com.gagara.homekeeper.utils;

import static com.gagara.homekeeper.common.Constants.REQUEST_ENABLE_BT;

import java.util.Collections;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Intent;

public class BluetoothUtils {

    private static BluetoothAdapter bluetoothAdapter = null;

    static {
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    }

    public static boolean isEnabled() {
        if (bluetoothAdapter != null) {
            return bluetoothAdapter.isEnabled();
        }
        return false;
    }

    public static boolean enableBluetooth(Activity ctx) {
        if (bluetoothAdapter != null) {
            if (!bluetoothAdapter.isEnabled()) {
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                ctx.startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
            }
            return true;
        } else {
            // Device does not support Bluetooth
            return false;
        }
    }

    public static Map<String, String> getPairedDevicesMap() {
        Map<String, String> devs = null;
        Set<BluetoothDevice> pairedDevices = getPairedDevices();
        if (pairedDevices != null) {
            if (pairedDevices.size() > 0) {
                devs = new TreeMap<String, String>();
                for (BluetoothDevice d : pairedDevices) {
                    devs.put(d.getAddress(), d.getName() + '\n' + "[" + d.getAddress() + "]");
                }
            } else {
                devs = Collections.emptyMap();
            }
        }
        return devs;
    }

    public static Set<BluetoothDevice> getPairedDevices() {
        Set<BluetoothDevice> result = null;
        if (bluetoothAdapter != null) {
            result = bluetoothAdapter.getBondedDevices();
        }
        return result;
    }

    public static int getPairedDevicesCount() {
        Set<BluetoothDevice> devs = getPairedDevices();
        if (devs != null) {
            return devs.size();
        } else {
            return 0;
        }
    }
}

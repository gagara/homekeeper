package com.gagara.homekeeper.activity;

import static android.widget.Toast.LENGTH_LONG;
import static com.gagara.homekeeper.common.Constants.CFG_BT_DEV;
import static com.gagara.homekeeper.common.Constants.CFG_MODE;
import static com.gagara.homekeeper.common.Constants.REQUEST_ENABLE_BT;

import java.util.Collections;
import java.util.Map;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.EditTextPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.widget.Toast;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.Mode;
import com.gagara.homekeeper.utils.BluetoothUtils;
import com.gagara.homekeeper.utils.HomeKeeperConfig;

public class SettingsActivity extends PreferenceActivity implements OnSharedPreferenceChangeListener {

    private ListPreference modeList;
    private ListPreference btDevList;
    private EditTextPreference proxyHost;
    private EditTextPreference proxyPort;
    private EditTextPreference proxyUser;
    private EditTextPreference proxyPassword;
    private ListPreference proxyPullPeriod;

    @SuppressWarnings("deprecation")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.layout.preferences);

        modeList = (ListPreference) findPreference(CFG_MODE);
        btDevList = (ListPreference) findPreference(CFG_BT_DEV);
        proxyHost = (EditTextPreference) findPreference(Constants.CFG_PROXY_HOST);
        proxyPort = (EditTextPreference) findPreference(Constants.CFG_PROXY_PORT);
        proxyUser = (EditTextPreference) findPreference(Constants.CFG_PROXY_USER);
        proxyPassword = (EditTextPreference) findPreference(Constants.CFG_PROXY_PASSWORD);
        proxyPullPeriod = (ListPreference) findPreference(Constants.CFG_PROXY_PULL_PERIOD);

        initComponents();

        BroadcastReceiver btStateChangedReceiver = new BroadcastReceiver() {
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)) {
                    refreshBtDevListControl(null);
                }
            }
        };
        registerReceiver(btStateChangedReceiver, new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED));
    }

    @Override
    protected void onResume() {
        super.onResume();

        PreferenceManager.getDefaultSharedPreferences(this).registerOnSharedPreferenceChangeListener(this);

        // refresh components
        refreshModeControl(null);
        refreshBtDevListControl(null);
        refreshProxyHostControl(null);
        refreshProxyPortControl(null);
        refreshProxyUserControl(null);
        refreshProxyPasswordControl(null);
        refreshProxyPullPeriodControl(null);
    }
    
    @Override
    protected void onPause() {
        super.onPause();
        PreferenceManager.getDefaultSharedPreferences(this).unregisterOnSharedPreferenceChangeListener(this);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (REQUEST_ENABLE_BT == requestCode) {
            if (RESULT_OK == resultCode) {
                refreshBtDevListControl(null);
            } else {
                Toast.makeText(this, R.string.bt_disabled_error, LENGTH_LONG).show();
            }
        } else {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        if (key.equals(Constants.CFG_MODE)) {
            if (HomeKeeperConfig.getMode(this) == Mode.DIRECT) {
                BluetoothUtils.enableBluetooth(this);
            }
        }
    }

    private void initComponents() {
        // mode
        modeList.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                refreshModeControl((String) newValue);
                return true;
            }
        });

        // btDevList
        btDevList.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                refreshBtDevListControl((String) newValue);
                return true;
            }
        });


        // proxyHost
        proxyHost.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                refreshProxyHostControl((String) newValue);
                return true;
            }
        });

        // proxyPort
        proxyPort.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                refreshProxyPortControl((Integer) newValue);
                return true;
            }
        });

        // proxyUser
        proxyUser.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                refreshProxyUserControl((String) newValue);
                return true;
            }
        });

        // proxyPassword
        proxyPassword.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                refreshProxyPasswordControl((String) newValue);
                return true;
            }
        });

        // proxyPullPeriod
        proxyPullPeriod.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                refreshProxyPullPeriodControl((Integer) newValue);
                return true;
            }
        });
    }

    private void refreshModeControl(String newValue) {
        Mode newMode = Mode.fromString(newValue);
        if (newMode == null) {
            newMode = HomeKeeperConfig.getMode(this);
        }
        if (newMode != null) {
            modeList.setSummary(newMode.name());
        } else {
            modeList.setSummary(R.string.pref_mode_default_summary);
        }
    }

    private void refreshBtDevListControl(String newValue) {
        updateBtDevList();
        updateBtDevListSummary(newValue);
        if (BluetoothUtils.isEnabled() && BluetoothUtils.getPairedDevicesCount() > 0) {
            btDevList.setEnabled(true);
        } else {
            btDevList.setEnabled(false);
        }
    }

    private void refreshProxyHostControl(String newValue) {
        if (newValue != null) {
            proxyHost.setSummary(newValue);
        } else {
            proxyHost.setSummary(R.string.pref_proxy_host_default_summary);
        }
    }

    private void refreshProxyPortControl(Integer newValue) {
        if (newValue != null) {
            proxyPort.setSummary(newValue);
        } else {
            proxyPort.setSummary(R.string.pref_proxy_port_default_summary);
        }
    }

    private void refreshProxyUserControl(String newValue) {
        if (newValue != null) {
            proxyUser.setSummary(newValue);
        } else {
            proxyUser.setSummary(R.string.pref_proxy_user_default_summary);
        }
    }

    private void refreshProxyPasswordControl(String newValue) {
        if (newValue != null) {
            proxyPassword.setSummary(R.string.pref_proxy_password_non_empty_summary);
        } else {
            proxyPassword.setSummary(R.string.pref_proxy_password_default_summary);
        }
    }

    private void refreshProxyPullPeriodControl(Integer newValue) {
        if (newValue != null) {
            proxyPullPeriod.setSummary(newValue);
        } else {
            proxyPullPeriod.setSummary(R.string.pref_proxy_pull_period_default_summary);
        }
    }

    private void updateBtDevList() {
        Map<String, String> availableBtDevices = BluetoothUtils.getPairedDevicesMap();
        if (availableBtDevices != null && availableBtDevices.size() > 0) {
            btDevList.setEntries(availableBtDevices.values().toArray(new String[] {}));
            btDevList.setEntryValues(availableBtDevices.keySet().toArray(new String[] {}));
        } else {
            btDevList.setEntries(Collections.emptyList().toArray(new String[] {}));
            btDevList.setEntryValues(Collections.emptyList().toArray(new String[] {}));
        }
    }

    private void updateBtDevListSummary(String newValue) {
        if (BluetoothUtils.isEnabled()) {
            Map<String, String> availableBtDevices = BluetoothUtils.getPairedDevicesMap();
            if (availableBtDevices != null && availableBtDevices.size() > 0) {
                String btDev = null;
                if (newValue == null) {
                    BluetoothDevice dev = HomeKeeperConfig.getBluetoothDevice(this);
                    if (dev != null) {
                        btDev = dev.getAddress();
                    }
                } else {
                    btDev = newValue;
                }
                if (btDev == null || btDev.equals("")) {
                    btDevList.setSummary(R.string.pref_bt_device_default_summary);
                } else {
                    if (availableBtDevices.containsKey(btDev)) {
                        btDevList.setSummary(availableBtDevices.get(btDev));
                    } else {
                        btDevList.setSummary(R.string.pref_bt_device_default_summary);
                    }
                }
            } else {
                btDevList.setSummary(R.string.bt_no_paired_devs_error);
            }
        } else {
            // bt is disabled or not available
            btDevList.setSummary(R.string.bt_disabled_error);
        }
    }
}

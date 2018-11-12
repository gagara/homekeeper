package com.gagara.homekeeper.activity;

import static com.gagara.homekeeper.common.Constants.CFG_ADD_GATEWAY;
import static com.gagara.homekeeper.common.Constants.CFG_GATEWAY_CATEGORY;
import android.annotation.SuppressLint;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;

import com.gagara.homekeeper.R;

@SuppressLint("NewApi")
@SuppressWarnings("deprecation")
public class Settings extends PreferenceActivity {

    // private EditTextPreference gatewayHost;
    // private EditTextPreference gatewayPort;
    // private EditTextPreference gatewayUser;
    // private EditTextPreference gatewayPassword;
    // private ListPreference gatewayPullPeriod;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.layout.preferences);

        PreferenceCategory gwCategory = (PreferenceCategory) findPreference(CFG_GATEWAY_CATEGORY);
        Preference addNewGatewayItem = new Preference(this);
        addNewGatewayItem.setKey(CFG_ADD_GATEWAY);
        addNewGatewayItem.setTitle(R.string.pref_add_gateway_title);
        addNewGatewayItem.setIcon(R.drawable.ic_menu_btn_add);
        addNewGatewayItem.setOnPreferenceClickListener(new OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                // TODO handler
                return false;
            }
        });
        gwCategory.addPreference(addNewGatewayItem);

        // PreferenceManager.getDefaultSharedPreferences(this).edit().
        // PreferenceManager.getDefaultSharedPreferences(this).
        // gatewayHost = (EditTextPreference)
        // findPreference(Constants.CFG_GATEWAY_HOST);
        // gatewayPort = (EditTextPreference)
        // findPreference(Constants.CFG_GATEWAY_PORT);
        // gatewayUser = (EditTextPreference)
        // findPreference(Constants.CFG_GATEWAY_USER);
        // gatewayPassword = (EditTextPreference)
        // findPreference(Constants.CFG_GATEWAY_PASSWORD);
        // gatewayPullPeriod = (ListPreference)
        // findPreference(Constants.CFG_GATEWAY_PULL_PERIOD);

        // initComponents();
    }

    @Override
    protected void onResume() {
        super.onResume();
        refreshGatewaysList((PreferenceCategory) findPreference(CFG_GATEWAY_CATEGORY));
        // PreferenceManager.getDefaultSharedPreferences(this).registerOnSharedPreferenceChangeListener(this);

        // refresh components
        // refreshGatewayHostControl(null);
        // refreshGatewayPortControl(null);
        // refreshGatewayUserControl(null);
        // refreshGatewayPasswordControl(null);
        // refreshGatewayPullPeriodControl(null);
    }

    @Override
    protected void onPause() {
        super.onPause();
        // PreferenceManager.getDefaultSharedPreferences(this).unregisterOnSharedPreferenceChangeListener(this);
    }

    private void refreshGatewaysList(PreferenceCategory category) {
        // TODO
    }
    //
    // @Override
    // protected void onActivityResult(int requestCode, int resultCode, Intent
    // data) {
    // if (REQUEST_ENABLE_NETWORK == requestCode) {
    // refreshGatewayHostControl(null);
    // refreshGatewayPortControl(null);
    // refreshGatewayUserControl(null);
    // refreshGatewayPasswordControl(null);
    // refreshGatewayPullPeriodControl(null);
    // } else {
    // super.onActivityResult(requestCode, resultCode, data);
    // }
    // }

    // private void initComponents() {
    // gatewayHost.setOnPreferenceChangeListener(new
    // OnPreferenceChangeListener() {
    // @Override
    // public boolean onPreferenceChange(Preference preference, Object newValue)
    // {
    // refreshGatewayHostControl((String) newValue);
    // return true;
    // }
    // });
    //
    // gatewayPort.setOnPreferenceChangeListener(new
    // OnPreferenceChangeListener() {
    // @Override
    // public boolean onPreferenceChange(Preference preference, Object newValue)
    // {
    // Integer port = null;
    // try {
    // port = Integer.parseInt((String) newValue);
    // } catch (NumberFormatException e) {
    // // ignore
    // }
    // refreshGatewayPortControl(port);
    // return true;
    // }
    // });
    //
    // gatewayUser.setOnPreferenceChangeListener(new
    // OnPreferenceChangeListener() {
    // @Override
    // public boolean onPreferenceChange(Preference preference, Object newValue)
    // {
    // refreshGatewayUserControl((String) newValue);
    // return true;
    // }
    // });
    //
    // gatewayPassword.setOnPreferenceChangeListener(new
    // OnPreferenceChangeListener() {
    // @Override
    // public boolean onPreferenceChange(Preference preference, Object newValue)
    // {
    // refreshGatewayPasswordControl((String) newValue);
    // return true;
    // }
    // });
    //
    // gatewayPullPeriod.setOnPreferenceChangeListener(new
    // OnPreferenceChangeListener() {
    // @Override
    // public boolean onPreferenceChange(Preference preference, Object newValue)
    // {
    // Integer period = null;
    // try {
    // period = Integer.parseInt((String) newValue);
    // } catch (NumberFormatException e) {
    // // ignore
    // }
    // refreshGatewayPullPeriodControl(period);
    // return true;
    // }
    // });
    // }
    //
    // private void refreshGatewayHostControl(String newValue) {
    // if (newValue == null) {
    // newValue = HomeKeeperConfig.getNbiGateway(this).getHost();
    // }
    // if (newValue != null && newValue.length() > 0) {
    // gatewayHost.setSummary(newValue);
    // } else {
    // gatewayHost.setSummary(R.string.pref_gateway_host_default_summary);
    // }
    // if (NetworkUtils.isEnabled(this)) {
    // gatewayHost.setEnabled(true);
    // } else {
    // gatewayHost.setEnabled(false);
    // }
    // }
    //
    // private void refreshGatewayPortControl(Integer newValue) {
    // if (newValue == null) {
    // newValue = HomeKeeperConfig.getNbiGateway(this).getPort();
    // }
    // if (newValue != null && newValue > 0) {
    // gatewayPort.setSummary(newValue.toString());
    // } else {
    // gatewayPort.setSummary(R.string.pref_gateway_port_default_summary);
    // }
    // if (NetworkUtils.isEnabled(this)) {
    // gatewayPort.setEnabled(true);
    // } else {
    // gatewayPort.setEnabled(false);
    // }
    // }
    //
    // private void refreshGatewayUserControl(String newValue) {
    // if (newValue == null) {
    // newValue = HomeKeeperConfig.getNbiGateway(this).getUsername();
    // }
    // if (newValue != null && newValue.length() > 0) {
    // gatewayUser.setSummary(newValue);
    // } else {
    // gatewayUser.setSummary(R.string.pref_gateway_user_default_summary);
    // }
    // if (NetworkUtils.isEnabled(this)) {
    // gatewayUser.setEnabled(true);
    // } else {
    // gatewayUser.setEnabled(false);
    // }
    // }
    //
    // private void refreshGatewayPasswordControl(String newValue) {
    // if (newValue == null) {
    // newValue = HomeKeeperConfig.getNbiGateway(this).getPassword();
    // }
    // if (newValue != null && newValue.length() > 0) {
    // gatewayPassword.setSummary(R.string.pref_gateway_password_non_empty_summary);
    // } else {
    // gatewayPassword.setSummary(R.string.pref_gateway_password_default_summary);
    // }
    // if (NetworkUtils.isEnabled(this)) {
    // gatewayPassword.setEnabled(true);
    // } else {
    // gatewayPassword.setEnabled(false);
    // }
    // }
    //
    // private void refreshGatewayPullPeriodControl(Integer newValue) {
    // if (newValue == null) {
    // newValue = HomeKeeperConfig.getNbiGateway(this).getPullPeriod();
    // }
    // if (newValue != null) {
    // gatewayPullPeriod.setSummary(getResources().getStringArray(R.array.pref_gateway_pull_period_entries)[Arrays
    // .asList(getResources().getStringArray(R.array.pref_gateway_pull_period_entry_values)).indexOf(
    // newValue + "")]);
    // } else {
    // gatewayPullPeriod.setSummary(R.string.pref_gateway_pull_period_default_summary);
    // }
    // if (NetworkUtils.isEnabled(this)) {
    // gatewayPullPeriod.setEnabled(true);
    // } else {
    // gatewayPullPeriod.setEnabled(false);
    // }
    // }
}

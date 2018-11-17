package com.gagara.homekeeper.activity;

import static com.gagara.homekeeper.common.Constants.CFG_ADD_GATEWAY_OPTION_KEY;
import static com.gagara.homekeeper.common.Constants.CFG_GATEWAY_CATEGORY_KEY;

import java.util.Map;
import java.util.Map.Entry;

import android.annotation.SuppressLint;
import android.content.res.Configuration;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.Gateway;
import com.gagara.homekeeper.utils.HomeKeeperConfig;

@SuppressLint("NewApi")
@SuppressWarnings("deprecation")
public class Settings extends PreferenceActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.layout.preferences);
    }

    @Override
    protected void onResume() {
        super.onResume();
        refreshGatewaysList();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    public void refreshGatewaysList() {
        refreshGatewaysList((PreferenceCategory) findPreference(CFG_GATEWAY_CATEGORY_KEY));
    }

    private void refreshGatewaysList(PreferenceCategory category) {
        Map<String, Gateway> gateways = HomeKeeperConfig.getAllNbiGateways(this);
        category.removeAll();
        for (Entry<String, Gateway> e : gateways.entrySet()) {
            Preference preferenceItem = new GatewayConfigureDialog(this, null, e.getValue(), e.getKey());
            preferenceItem.setKey(Constants.CFG_GATEWAY_OPTION_KEY_PREFIX + e.getKey());
            preferenceItem.setTitle(e.getValue().getHost() + ":" + e.getValue().getPort());
            preferenceItem.setIcon(R.drawable.icons8_system_report);
            category.addPreference(preferenceItem);
        }
        // add "New GW" option
        Preference addNewGatewayItem = new GatewayConfigureDialog(this, null, null, null);
        addNewGatewayItem.setKey(CFG_ADD_GATEWAY_OPTION_KEY);
        addNewGatewayItem.setTitle(R.string.pref_add_gateway_title);
        addNewGatewayItem.setIcon(R.drawable.ic_menu_btn_add);
        category.addPreference(addNewGatewayItem);
    }
}

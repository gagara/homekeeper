package com.gagara.homekeeper.utils;

import static com.gagara.homekeeper.common.Constants.CFG_GATEWAY_HOST;
import static com.gagara.homekeeper.common.Constants.CFG_GATEWAY_PORT;
import android.content.Context;
import android.preference.PreferenceManager;

import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.Gateway;

public class HomeKeeperConfig {

    private HomeKeeperConfig() {
    }

    public static Gateway getNbiGateway(Context ctx) {
        return getNbiGateway(ctx, null);
    }

    public static Gateway getNbiGateway(Context ctx, Gateway defValue) {
        String host = PreferenceManager.getDefaultSharedPreferences(ctx).getString(CFG_GATEWAY_HOST,
                defValue != null ? defValue.getHost() : null);
        Integer port = null;
        try {
            port = Integer.parseInt(PreferenceManager.getDefaultSharedPreferences(ctx).getString(CFG_GATEWAY_PORT,
                    defValue != null ? defValue.getPort() + "" : "0"));
        } catch (NumberFormatException e) {
            // ignore
        }
        String username = PreferenceManager.getDefaultSharedPreferences(ctx).getString(Constants.CFG_GATEWAY_USER,
                defValue != null ? defValue.getUsername() : null);
        String password = PreferenceManager.getDefaultSharedPreferences(ctx).getString(Constants.CFG_GATEWAY_PASSWORD,
                defValue != null ? defValue.getPassword() : null);
        Integer period = null;
        try {
            period = Integer.parseInt(PreferenceManager.getDefaultSharedPreferences(ctx).getString(
                    Constants.CFG_GATEWAY_PULL_PERIOD, defValue != null ? defValue.getPullPeriod() + "" : "0"));
        } catch (NumberFormatException e) {
            // ignore
        }
        return new Gateway(host, port, username, password, period);
    }

}

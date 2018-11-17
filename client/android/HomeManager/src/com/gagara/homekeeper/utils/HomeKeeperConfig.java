package com.gagara.homekeeper.utils;

import static com.gagara.homekeeper.common.Constants.CFG_AVAILABLE_GATEWAY_IDS;
import static com.gagara.homekeeper.common.Constants.CFG_GATEWAY_HOST_PREFIX;
import static com.gagara.homekeeper.common.Constants.CFG_GATEWAY_PASSWORD_PREFIX;
import static com.gagara.homekeeper.common.Constants.CFG_GATEWAY_PORT_PREFIX;
import static com.gagara.homekeeper.common.Constants.CFG_GATEWAY_PULL_PERIOD_PREFIX;
import static com.gagara.homekeeper.common.Constants.CFG_GATEWAY_USER_PREFIX;
import static com.gagara.homekeeper.common.Constants.CFG_SELECTED_GATEWAY_ID;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.UUID;

import android.content.Context;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;

import com.gagara.homekeeper.common.Gateway;

public class HomeKeeperConfig {

    private HomeKeeperConfig() {
    }

    public static Map<String, Gateway> getAllNbiGateways(Context ctx) {
        Map<String, Gateway> result = new TreeMap<String, Gateway>();
        List<String> ids = new ArrayList<String>();
        String availableIds = PreferenceManager.getDefaultSharedPreferences(ctx).getString(CFG_AVAILABLE_GATEWAY_IDS,
                "");
        if (availableIds.length() > 0) {
            ids.addAll(Arrays.asList(availableIds.split(";")));
        }
        for (String id : ids) {
            String host = PreferenceManager.getDefaultSharedPreferences(ctx)
                    .getString(CFG_GATEWAY_HOST_PREFIX + id, "");
            Integer port = PreferenceManager.getDefaultSharedPreferences(ctx).getInt(CFG_GATEWAY_PORT_PREFIX + id, 0);
            String user = PreferenceManager.getDefaultSharedPreferences(ctx)
                    .getString(CFG_GATEWAY_USER_PREFIX + id, "");
            String password = PreferenceManager.getDefaultSharedPreferences(ctx).getString(
                    CFG_GATEWAY_PASSWORD_PREFIX + id, "");
            Integer pullPeriod = PreferenceManager.getDefaultSharedPreferences(ctx).getInt(
                    CFG_GATEWAY_PULL_PERIOD_PREFIX + id, 0);

            Gateway gw = new Gateway(host, port, user, password, pullPeriod);
            result.put(id, gw);
        }
        return result;
    }

    public static void addNbiGateways(Context ctx, Gateway gateway) {
        Editor editor = PreferenceManager.getDefaultSharedPreferences(ctx).edit();
        String id = new Date().getTime() + "_" + UUID.randomUUID().toString().substring(0, 6);
        List<String> ids = new ArrayList<String>();
        String availableIds = PreferenceManager.getDefaultSharedPreferences(ctx).getString(CFG_AVAILABLE_GATEWAY_IDS,
                "");
        if (availableIds.length() > 0) {
            ids.addAll(Arrays.asList(availableIds.split(";")));
        }
        ids.add(id);
        StringBuilder idsStr = new StringBuilder();
        for (String i : ids) {
            idsStr.append(i).append(";");
        }
        editor.putString(CFG_AVAILABLE_GATEWAY_IDS, idsStr.substring(0, idsStr.length() - 1));
        editor.putString(CFG_GATEWAY_HOST_PREFIX + id, gateway.getHost());
        editor.putInt(CFG_GATEWAY_PORT_PREFIX + id, gateway.getPort());
        editor.putString(CFG_GATEWAY_USER_PREFIX + id, gateway.getUsername());
        editor.putString(CFG_GATEWAY_PASSWORD_PREFIX + id, gateway.getPassword());
        editor.putInt(CFG_GATEWAY_PULL_PERIOD_PREFIX + id, gateway.getPullPeriod());
        editor.commit();
    }

    public static void updateNbiGateways(Context ctx, Gateway gateway, String id) {
        Editor editor = PreferenceManager.getDefaultSharedPreferences(ctx).edit();
        List<String> ids = Arrays.asList(PreferenceManager.getDefaultSharedPreferences(ctx)
                .getString(CFG_AVAILABLE_GATEWAY_IDS, "").split(";"));
        if (ids.contains(id)) {
            editor.putString(CFG_GATEWAY_HOST_PREFIX + id, gateway.getHost());
            editor.putInt(CFG_GATEWAY_PORT_PREFIX + id, gateway.getPort());
            editor.putString(CFG_GATEWAY_USER_PREFIX + id, gateway.getUsername());
            editor.putString(CFG_GATEWAY_PASSWORD_PREFIX + id, gateway.getPassword());
            editor.putInt(CFG_GATEWAY_PULL_PERIOD_PREFIX + id, gateway.getPullPeriod());
        }
        editor.commit();
    }

    public static void deleteNbiGateways(Context ctx, String id) {
        Editor editor = PreferenceManager.getDefaultSharedPreferences(ctx).edit();
        List<String> ids = new ArrayList<String>();
        String availableIds = PreferenceManager.getDefaultSharedPreferences(ctx).getString(CFG_AVAILABLE_GATEWAY_IDS,
                "");
        if (availableIds.length() > 0) {
            ids.addAll(Arrays.asList(availableIds.split(";")));
        }
        if (ids.contains(id)) {
            ids.remove(id);
            StringBuilder idsStr = new StringBuilder();
            for (String i : ids) {
                idsStr.append(i).append(";");
            }
            editor.putString(CFG_AVAILABLE_GATEWAY_IDS, idsStr.length() > 0 ? idsStr.substring(0, idsStr.length() - 1)
                    : null);
            editor.remove(CFG_GATEWAY_HOST_PREFIX + id);
            editor.remove(CFG_GATEWAY_PORT_PREFIX + id);
            editor.remove(CFG_GATEWAY_USER_PREFIX + id);
            editor.remove(CFG_GATEWAY_PASSWORD_PREFIX + id);
            editor.remove(CFG_GATEWAY_PULL_PERIOD_PREFIX + id);
        }
        editor.commit();
    }

    public static Gateway getNbiGateway(Context ctx) {
        Map<String, Gateway> allGateways = getAllNbiGateways(ctx);
        String selectedId = PreferenceManager.getDefaultSharedPreferences(ctx).getString(CFG_SELECTED_GATEWAY_ID, null);
        if (selectedId != null) {
            return allGateways.get(selectedId);
        } else {
            return null;
        }
    }

}

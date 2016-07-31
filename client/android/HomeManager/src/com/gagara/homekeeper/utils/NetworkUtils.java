package com.gagara.homekeeper.utils;

import static android.widget.Toast.LENGTH_LONG;
import static com.gagara.homekeeper.common.Constants.REQUEST_ENABLE_NETWORK;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.provider.Settings;
import android.widget.Toast;

import com.gagara.homekeeper.R;

public class NetworkUtils {

    public static boolean enableNetwork(final Context ctx) {
        ConnectivityManager cm = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo netInfo = cm.getActiveNetworkInfo();
        if (netInfo == null) {
            AlertDialog.Builder builder = new AlertDialog.Builder(ctx);
            builder.setTitle(R.string.manage_networks_connectivity_required_title);
            builder.setMessage(R.string.manage_networks_connectivity_required_message);
            builder.setCancelable(false);
            builder.setPositiveButton(R.string.manage_networks_button_ok, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    Intent settings = new Intent(Settings.ACTION_WIRELESS_SETTINGS);
                    ((Activity) ctx).startActivityForResult(settings, REQUEST_ENABLE_NETWORK);
                }
            });
            builder.setNegativeButton(R.string.manage_networks_button_cancel, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    Toast.makeText(ctx, R.string.networks_disabled_error, LENGTH_LONG).show();
                }
            });

            builder.create().show();

        }
        return true;
    }

    public static boolean isEnabled(Context ctx) {
        ConnectivityManager cm = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo netInfo = cm.getActiveNetworkInfo();
        return (netInfo != null && netInfo.isConnected());
    }
}

package com.gagara.homekeeper.activity;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.DialogInterface;
import android.preference.DialogPreference;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnAttachStateChangeListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.common.Gateway;
import com.gagara.homekeeper.utils.HomeKeeperConfig;

@SuppressLint("NewApi")
public class GatewayConfigureDialog extends DialogPreference implements TextWatcher, OnAttachStateChangeListener {

    private Context ctx;

    private Gateway gateway = null;
    private String id = null;

    private EditText hostView;
    private EditText portView;
    private EditText userView;
    private EditText passwordView;
    private Spinner pullPeriodView;

    public GatewayConfigureDialog(Context context, AttributeSet attrs, Gateway gateway, String id) {
        super(context, attrs);
        this.ctx = context;
        this.gateway = gateway;
        this.id = id;
        setPersistent(false);
        setDialogLayoutResource(R.layout.gateway_settings);
    }

    @Override
    protected void onPrepareDialogBuilder(Builder builder) {
        super.onPrepareDialogBuilder(builder);
        if (gateway != null) {
            builder.setPositiveButton(R.string.action_save, this);
            builder.setNegativeButton(R.string.action_delete, this);
        } else {
            builder.setPositiveButton(R.string.action_add, this);
            builder.setNegativeButton(null, this);
        }
        builder.setNeutralButton(R.string.action_cancel, this);
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);
        hostView = (EditText) view.findViewById(R.id.gateway_host);
        portView = (EditText) view.findViewById(R.id.gateway_port);
        userView = (EditText) view.findViewById(R.id.gateway_user);
        passwordView = (EditText) view.findViewById(R.id.gateway_password);
        pullPeriodView = (Spinner) view.findViewById(R.id.gateway_pull_period_list);

        hostView.addTextChangedListener(this);
        portView.addTextChangedListener(this);
        userView.addTextChangedListener(this);
        passwordView.addTextChangedListener(this);

        hostView.addOnAttachStateChangeListener(this);

        if (gateway != null) {
            hostView.setText(gateway.getHost());
            portView.setText(gateway.getPort() + "");
            userView.setText(gateway.getUsername());
            passwordView.setText(gateway.getPassword());
            int[] periods = ctx.getResources().getIntArray(R.array.pref_gateway_pull_period_entry_values);
            for (int i = 0; i < periods.length; i++) {
                if (periods[i] == gateway.getPullPeriod()) {
                    pullPeriodView.setSelection(i);
                    break;
                }
            }
        }
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        switch (which) {
        case DialogInterface.BUTTON_POSITIVE:
            int[] periods = ctx.getResources().getIntArray(R.array.pref_gateway_pull_period_entry_values);
            if (gateway == null) {
                // Add
                gateway = new Gateway(hostView.getText().toString(), Integer.parseInt(portView.getText().toString()),
                        userView.getText().toString(), passwordView.getText().toString(),
                        (Integer) periods[pullPeriodView.getSelectedItemPosition()]);
                HomeKeeperConfig.addNbiGateways(ctx, gateway);
            } else {
                // Update
                gateway = new Gateway(hostView.getText().toString(), Integer.parseInt(portView.getText().toString()),
                        userView.getText().toString(), passwordView.getText().toString(),
                        (Integer) periods[pullPeriodView.getSelectedItemPosition()]);
                HomeKeeperConfig.updateNbiGateways(ctx, gateway, id);
            }
            break;

        case DialogInterface.BUTTON_NEGATIVE:
            // Delete
            HomeKeeperConfig.deleteNbiGateways(ctx, id);
            break;

        default:
            break;
        }
        ((Settings) ctx).refreshGatewaysList();
        super.onClick(dialog, which);
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
    }

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) {
    }

    @Override
    public void afterTextChanged(Editable s) {
        if (getDialog() != null) {
            Button button = ((AlertDialog) getDialog()).getButton(AlertDialog.BUTTON_POSITIVE);
            if (s.length() == 0) {
                button.setEnabled(false);
                return;
            }
            if (hostView != null && hostView.getText().length() == 0) {
                button.setEnabled(false);
                return;
            }
            if (portView != null && portView.getText().length() == 0) {
                button.setEnabled(false);
                return;
            }
            if (userView != null && userView.getText().length() == 0) {
                button.setEnabled(false);
                return;
            }
            if (passwordView != null && passwordView.getText().length() == 0) {
                button.setEnabled(false);
                return;
            }
            button.setEnabled(true);
        }
    }

    @Override
    public void onViewAttachedToWindow(View v) {
        if (gateway == null) {
            ((AlertDialog) getDialog()).getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(false);
        }
    }

    @Override
    public void onViewDetachedFromWindow(View v) {
    }
}

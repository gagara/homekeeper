package com.gagara.homekeeper.service;

import static com.gagara.homekeeper.common.Constants.COMMAND_KEY;
import static com.gagara.homekeeper.common.Constants.CONTROLLER_CONTROL_COMMAND_ACTION;
import static com.gagara.homekeeper.common.Constants.CONTROLLER_SERVICE_ONGOING_NOTIFICATION_ID;

import java.io.IOException;
import java.util.Date;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ScheduledExecutorService;

import org.json.JSONException;
import org.json.JSONObject;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.support.v4.app.NotificationCompat;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.activity.MainActivity;
import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.nbi.request.ClockSyncRequest;
import com.gagara.homekeeper.nbi.request.CurrentStatusRequest;
import com.gagara.homekeeper.nbi.request.Request;
import com.gagara.homekeeper.nbi.response.CurrentStatusResponse;
import com.gagara.homekeeper.nbi.response.NodeStateChangeResponse;

public abstract class AbstractNbiService extends Service {

    private static final String TAG = AbstractNbiService.class.getName();

    protected static final long INITIAL_CLOCK_SYNC_INTERVAL_SEC = 10;

    protected volatile State state;

    protected int startId;

    protected Date lastMessageTimestamp = new Date(0);
    protected Long clocksDelta = null;

    private NotificationCompat.Builder serviceNotification;

    protected ExecutorService serviceExecutor = null;
    protected BroadcastReceiver controllerCommandReceiver = null;
    protected ScheduledExecutorService clockSyncExecutor = null;

    private Future<?> currentTask = null;

    public enum State {
        INIT, ACTIVE, ERROR, SHUTDOWN;

        public int toStringResource() {
            if (this == INIT) {
                return R.string.service_state_init;
            } else if (this == ACTIVE) {
                return R.string.service_state_active;
            } else if (this == ERROR) {
                return R.string.service_state_error;
            } else if (this == SHUTDOWN) {
                return R.string.service_state_shutdown;
            } else {
                return -1;
            }
        }

        public static State fromString(String str) {
            for (State s : values()) {
                if (s.toString().equalsIgnoreCase(str)) {
                    return s;
                }
            }
            return null;
        }
    }

    abstract void send(Request request);

    abstract String getServiceProviderName();

    abstract void setupService();

    @Override
    public void onCreate() {
        state = State.SHUTDOWN;
        startId = 0;
        serviceExecutor = Executors.newSingleThreadExecutor();
        controllerCommandReceiver = new ControllerCommandsReceiver();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, final int startId) {
        if ((currentTask == null || currentTask.isDone()) && !serviceExecutor.isTerminated()) {
            currentTask = serviceExecutor.submit(new Runnable() {
                @Override
                public void run() {
                    setupService();
                }
            });
            this.startId = startId;
            Log.i(TAG, "started with ID: " + this.startId);
        }
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onDestroy() {
        serviceNotification = null;
        startId = 0;
        state = State.SHUTDOWN;
        notifyStatusChange(null);
        serviceExecutor.shutdownNow();
        if (clockSyncExecutor != null) {
            clockSyncExecutor.shutdownNow();
        }
        LocalBroadcastManager.getInstance(this).unregisterReceiver(controllerCommandReceiver);
    }

    protected void initOngoingNotification() {
        PendingIntent pendingIntent = PendingIntent.getActivity(this, Constants.CONTROLLER_SERVICE_PENDING_INTENT_ID,
                new Intent(this, MainActivity.class), PendingIntent.FLAG_UPDATE_CURRENT);
        serviceNotification = new NotificationCompat.Builder(this).setContentTitle(getText(R.string.app_name))
                .setSmallIcon(R.drawable.ic_launcher);
        if (getServiceProviderName() != null) {
            serviceNotification.setContentText(getServiceProviderName() + ": " + getText(state.toStringResource()));
        } else {
            serviceNotification.setContentText(getText(R.string.unknown_service_provider));
        }
        serviceNotification.setContentIntent(pendingIntent);
        startForeground(CONTROLLER_SERVICE_ONGOING_NOTIFICATION_ID, serviceNotification.build());
    }

    protected void processMessage(JSONObject message, Date timestamp) throws JSONException, IOException {
        lastMessageTimestamp = new Date();
        ControllerConfig.MessageType msgType = ControllerConfig.MessageType.forCode(message
                .getString(ControllerConfig.MSG_TYPE_KEY));
        if (msgType == ControllerConfig.MessageType.CLOCK_SYNC) {
            long controllerTimestamp = message.getLong(ControllerConfig.TIMESTAMP_KEY);
            int overflowCount = message.getInt(ControllerConfig.OVERFLOW_COUNT_KEY);
            long delta = timestamp.getTime() - (controllerTimestamp + overflowCount * (long) Math.pow(2, 32));
            clockSyncExecutor.shutdownNow();
            notifyStatusChange(getText(R.string.service_listening_status));
            if (clocksDelta == null) {
                // initial sync: request for status
                CurrentStatusRequest csr = new CurrentStatusRequest();
                send(csr);
            }
            clocksDelta = delta;
        } else {
            if (clocksDelta != null) {
                Intent intent = new Intent();
                intent.setAction(Constants.CONTROLLER_DATA_TRANSFER_ACTION);
                if (msgType == ControllerConfig.MessageType.CURRENT_STATUS_REPORT) {
                    CurrentStatusResponse stats = new CurrentStatusResponse(clocksDelta).fromJson(message);
                    if (stats != null) {
                        intent.putExtra(Constants.DATA_KEY, stats);
                        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
                        notifyStatusChange(getText(R.string.service_receiving_data_status));
                    }
                } else if (msgType == ControllerConfig.MessageType.NODE_STATE_CHANGED) {
                    NodeStateChangeResponse nodeState = new NodeStateChangeResponse(clocksDelta).fromJson(message);
                    if (nodeState != null) {
                        intent.putExtra(Constants.DATA_KEY, nodeState);
                        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
                        notifyStatusChange(getText(R.string.service_receiving_data_status));
                    }
                } else {
                    // unknown message
                    Log.w(TAG, "ignore message type:" + msgType);
                }
            } else {
                // ignore any message until clocks not synchronized
                Log.w(TAG, "clocks not synchronized. ignoring message: " + message.toString());
            }
        }
    }

    protected final void notifyStatusChange(CharSequence details) {
        if (serviceNotification != null) {
            NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            if (getServiceProviderName() != null) {
                serviceNotification.setContentText(getServiceProviderName() + ": " + getText(state.toStringResource()));
            } else {
                serviceNotification.setContentText(getText(R.string.unknown_service_provider));
            }
            manager.notify(CONTROLLER_SERVICE_ONGOING_NOTIFICATION_ID, serviceNotification.build());
        }
        Intent intent = new Intent();
        intent.setAction(Constants.SERVICE_STATUS_ACTION);
        intent.putExtra(Constants.SERVICE_STATUS_KEY, state.toString());
        if (details != null) {
            intent.putExtra(Constants.SERVICE_STATUS_DETAILS_KEY, details);
        }
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    public class ControllerCommandsReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (CONTROLLER_CONTROL_COMMAND_ACTION.equals(action)) {
                Request command = intent.getParcelableExtra(COMMAND_KEY);
                send(command);
            }
        }
    }

    protected class SyncClockRequest implements Runnable {
        @Override
        public void run() {
            try {
                ClockSyncRequest req = new ClockSyncRequest();
                send(req);
            } catch (Exception e) {
                Log.e(TAG, "failed to sync clocks: " + e.getMessage(), e);
            }
        }
    }

}

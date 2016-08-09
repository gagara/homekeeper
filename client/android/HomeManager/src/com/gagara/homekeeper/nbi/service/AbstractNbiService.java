package com.gagara.homekeeper.nbi.service;

import static com.gagara.homekeeper.common.Constants.COMMAND_KEY;
import static com.gagara.homekeeper.common.Constants.CONTROLLER_CONTROL_COMMAND_ACTION;
import static com.gagara.homekeeper.common.Constants.CONTROLLER_SERVICE_ONGOING_NOTIFICATION_ID;
import static com.gagara.homekeeper.common.Constants.SERVICE_STATUS_CHANGE_ACTION;
import static com.gagara.homekeeper.common.Constants.SERVICE_STATUS_DETAILS_KEY;
import static com.gagara.homekeeper.common.Constants.SERVICE_STATUS_KEY;
import static com.gagara.homekeeper.common.Constants.SERVICE_TITLE_CHANGE_ACTION;
import static com.gagara.homekeeper.common.Constants.SERVICE_TITLE_KEY;
import static com.gagara.homekeeper.nbi.service.ServiceState.ACTIVE;
import static com.gagara.homekeeper.nbi.service.ServiceState.INIT;
import static com.gagara.homekeeper.nbi.service.ServiceState.SHUTDOWN;

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
import com.gagara.homekeeper.ui.view.ViewUtils;
import com.gagara.homekeeper.ui.viewmodel.TopModelView;

public abstract class AbstractNbiService extends Service {

    private static final String TAG = AbstractNbiService.class.getName();

    protected static final long INITIAL_CLOCK_SYNC_INTERVAL_SEC = 10;

    protected volatile ServiceState state;

    protected int startId;

    protected volatile Date lastMessageTimestamp = new Date(0);
    protected Long clocksDelta = null;

    private NotificationCompat.Builder serviceNotification;

    protected ExecutorService serviceExecutor = null;
    protected BroadcastReceiver controllerCommandReceiver = null;
    protected ScheduledExecutorService clockSyncExecutor = null;

    private Future<?> currentTask = null;

    abstract void send(Request request);

    abstract String getServiceProviderName();

    abstract void setupService();

    @Override
    public void onCreate() {
        state = INIT;
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

        // update title
        Intent titleUpdate = new Intent(SERVICE_TITLE_CHANGE_ACTION);
        titleUpdate.putExtra(SERVICE_TITLE_KEY, getServiceProviderName());
        LocalBroadcastManager.getInstance(this).sendBroadcast(titleUpdate);

        // update status
        notifyStatusChange(null);

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
        state = SHUTDOWN;
        // update status
        notifyStatusChange(null);
        // update title
        LocalBroadcastManager.getInstance(this).sendBroadcast(new Intent(SERVICE_TITLE_CHANGE_ACTION));
        serviceExecutor.shutdownNow();
        LocalBroadcastManager.getInstance(this).unregisterReceiver(controllerCommandReceiver);
        if (clockSyncExecutor != null) {
            clockSyncExecutor.shutdownNow();
        }
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
        lastMessageTimestamp = timestamp;
        ControllerConfig.MessageType msgType = ControllerConfig.MessageType.forCode(message
                .getString(ControllerConfig.MSG_TYPE_KEY));
        if (msgType == ControllerConfig.MessageType.CLOCK_SYNC) {
            synchronized (serviceExecutor) {
                if (clockSyncExecutor != null) {
                    clockSyncExecutor.shutdownNow();
                }
                clockSyncExecutor = null;
                long controllerTimestamp = message.getLong(ControllerConfig.TIMESTAMP_KEY);
                int overflowCount = message.getInt(ControllerConfig.OVERFLOW_COUNT_KEY);
                long delta = lastMessageTimestamp.getTime()
                        - (controllerTimestamp + overflowCount * (long) Math.pow(2, 32));
                if (clocksDelta == null) {
                    // initial sync: request for status
                    CurrentStatusRequest csr = new CurrentStatusRequest();
                    send(csr);
                }
                clocksDelta = delta;
                state = ACTIVE;
                notifyStatusChange(getText(R.string.service_listening_status));
            }
        } else {
            if (clocksDelta != null) {
                Intent intent = new Intent();
                intent.setAction(Constants.CONTROLLER_DATA_TRANSFER_ACTION);
                if (msgType == ControllerConfig.MessageType.CURRENT_STATUS_REPORT) {
                    CurrentStatusResponse stats = new CurrentStatusResponse(clocksDelta).fromJson(message);
                    if (stats != null) {
                        intent.putExtra(Constants.DATA_KEY, stats);
                        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);

                        String title = null;
                        String name = null;
                        if (stats.getNode() != null) {
                            title = getResources().getString(R.string.node_title);
                            if (ViewUtils.validNode(stats.getNode().getData())) {
                                name = getResources().getString(
                                        TopModelView.NODES_NAME_VIEW_MAP.get(stats.getNode().getData().getId()));
                            } else {
                                name = stats.getNode().getData().getId() + "";
                            }
                        } else if (stats.getSensor() != null) {
                            title = getResources().getString(R.string.sensor_title);
                            if (ViewUtils.validSensor(stats.getSensor().getData())) {
                                name = getResources().getString(
                                        TopModelView.SENSORS_NAME_VIEW_MAP.get(stats.getSensor().getData().getId()));
                            } else {
                                name = stats.getSensor().getData().getId() + "";
                            }
                        }

                        notifyStatusChange(String.format(getResources().getString(R.string.service_csr_message_status),
                                title, name));
                    }
                } else if (msgType == ControllerConfig.MessageType.NODE_STATE_CHANGED) {
                    NodeStateChangeResponse nodeState = new NodeStateChangeResponse(clocksDelta).fromJson(message);
                    if (nodeState != null) {
                        intent.putExtra(Constants.DATA_KEY, nodeState);
                        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);

                        String title = null;
                        String name = null;
                        title = getResources().getString(R.string.node_title);
                        if (ViewUtils.validNode(nodeState.getData())) {
                            name = getResources().getString(
                                    TopModelView.NODES_NAME_VIEW_MAP.get(nodeState.getData().getId()));
                        } else {
                            name = nodeState.getData().getId() + "";
                        }

                        notifyStatusChange(String.format(getResources().getString(R.string.service_nsc_message_status),
                                title, name));
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
        intent.setAction(SERVICE_STATUS_CHANGE_ACTION);
        intent.putExtra(SERVICE_STATUS_KEY, state.toString());
        if (details != null) {
            intent.putExtra(SERVICE_STATUS_DETAILS_KEY, details);
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

package com.gagara.homekeeper.service;

import static com.gagara.homekeeper.common.Constants.COMMAND_KEY;
import static com.gagara.homekeeper.common.Constants.CONTROLLER_CONTROL_COMMAND_ACTION;
import static com.gagara.homekeeper.common.Constants.CONTROLLER_SERVICE_ONGOING_NOTIFICATION_ID;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
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
import com.gagara.homekeeper.nbi.request.NodeStateChangeRequest;
import com.gagara.homekeeper.nbi.request.Request;
import com.gagara.homekeeper.nbi.response.CurrentStatusResponse;
import com.gagara.homekeeper.nbi.response.NodeStateChangeResponse;
import com.gagara.homekeeper.utils.HomeKeeperConfig;

public class BtCommunicationService extends Service {

    private static final String SP_UUID = "00001101-0000-1000-8000-00805F9B34FB";
    private static final String TAG = BtCommunicationService.class.getName();

    private static final long HEALTHCKECK_INTERVAL_SEC = 10;
    private static final long INITIAL_CLOCK_SYNC_INTERVAL_SEC = 10;

    private volatile State state;
    private int startId;
    private BluetoothDevice dev;

    private Long clocksDelta = null;
    private Date lastMessageTimestamp = new Date(0);

    private NotificationCompat.Builder serviceNotification;

    private Future<?> currentTask = null;

    private BluetoothSocket socket = null;
    private InputStream in = null;
    private OutputStream out = null;

    ExecutorService serviceExecutor = null;
    ScheduledExecutorService healthckeckExecutor = null;
    ScheduledExecutorService clockSyncExecutor = null;

    BroadcastReceiver controllerCommandReceiver = null;

    public enum State {
        INIT, ACTIVE, ERROR, SHUTDOWN;

        public int toStringResource() {
            if (this == INIT) {
                return R.string.bt_service_state_init;
            } else if (this == ACTIVE) {
                return R.string.bt_service_state_active;
            } else if (this == ERROR) {
                return R.string.bt_service_state_error;
            } else if (this == SHUTDOWN) {
                return R.string.bt_service_state_shutdown;
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

    @Override
    public void onCreate() {
        state = State.SHUTDOWN;
        startId = 0;
        serviceExecutor = Executors.newSingleThreadExecutor();
        healthckeckExecutor = Executors.newSingleThreadScheduledExecutor();
        controllerCommandReceiver = new ControllerCommandsReceiver();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, final int startId) {
        if ((currentTask == null || currentTask.isDone()) && !serviceExecutor.isTerminated()) {
            currentTask = serviceExecutor.submit(new Runnable() {
                @Override
                public void run() {
                    try {
                        dev = HomeKeeperConfig.getBluetoothDevice(BtCommunicationService.this);
                        initOngoingNotification();
                        if (dev != null) {
                            startHealthckecker();
                            runService();
                        }
                    } catch (InterruptedException e) {
                        // just terminating
                    }
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
        healthckeckExecutor.shutdownNow();
        if (clockSyncExecutor != null) {
            clockSyncExecutor.shutdownNow();
        }
        LocalBroadcastManager.getInstance(this).unregisterReceiver(controllerCommandReceiver);

        closeConnection();
    }

    private void initOngoingNotification() {
        PendingIntent pendingIntent = PendingIntent.getActivity(this,
                Constants.CONTROLLER_SERVICE_PENDING_INTENT_ID,
                new Intent(this, MainActivity.class), PendingIntent.FLAG_UPDATE_CURRENT);
        serviceNotification = new NotificationCompat.Builder(this).setContentTitle(
                getText(R.string.app_name)).setSmallIcon(R.drawable.ic_launcher);
        if (dev != null) {
            serviceNotification.setContentText(dev.getName() + ": "
                    + getText(state.toStringResource()));
        } else {
            serviceNotification.setContentText(getText(R.string.unknown_bt_dev));
        }
        serviceNotification.setContentIntent(pendingIntent);
        startForeground(CONTROLLER_SERVICE_ONGOING_NOTIFICATION_ID, serviceNotification.build());
    }

    private void startHealthckecker() {
        state = State.INIT;
        notifyStatusChange(null);
        if (!healthckeckExecutor.isTerminated()) {
            healthckeckExecutor.scheduleAtFixedRate(new Runnable() {
                @Override
                public void run() {
                    if (state == State.INIT) {
                        // connect
                        initConnection();
                    } else if (state == State.ERROR) {
                        // re-connection
                        initConnection();
                    } else if (state == State.ACTIVE) {
                        Date currTs = new Date();
                        if ((currTs.getTime() - lastMessageTimestamp.getTime()) > ControllerConfig.MAX_INACTIVE_PERIOD_SEC * 1000) {
                            closeConnection();
                            state = State.ERROR;
                        }
                    } else if (state == State.SHUTDOWN) {
                        // do nothing
                    }
                }
            }, 0L, HEALTHCKECK_INTERVAL_SEC, TimeUnit.SECONDS);
        }
    }

    private void initConnection() {
        try {
            BluetoothAdapter.getDefaultAdapter().cancelDiscovery();
            socket = dev.createRfcommSocketToServiceRecord(UUID.fromString(SP_UUID));
            socket.connect();
            in = socket.getInputStream();
            out = socket.getOutputStream();
            state = State.ACTIVE;
            lastMessageTimestamp = new Date();
            notifyStatusChange(null);
            synchronized (serviceExecutor) {
                serviceExecutor.notify();
            }
        } catch (IOException e) {
            Log.e(TAG, e.getMessage(), e);
            state = State.ERROR;
            notifyStatusChange(e.getMessage());
        }
    }

    private void closeConnection() {
        clocksDelta = null;
        if (in != null) {
            try {
                in.close();
            } catch (IOException e) {
                Log.w(TAG, e.getMessage(), e);
            }
        }
        if (out != null) {
            try {
                out.close();
            } catch (IOException e) {
                Log.w(TAG, e.getMessage(), e);
            }
        }
        if (socket != null) {
            try {
                socket.close();
            } catch (IOException e) {
                Log.w(TAG, e.getMessage(), e);
            }
        }
    }

    private void runService() throws InterruptedException {
        LocalBroadcastManager.getInstance(this).registerReceiver(controllerCommandReceiver,
                new IntentFilter(Constants.CONTROLLER_CONTROL_COMMAND_ACTION));
        while (true) {
            if (state == State.ACTIVE) {
                clockSyncExecutor = Executors.newSingleThreadScheduledExecutor();
                clockSyncExecutor.scheduleAtFixedRate(new SyncClockRequest(), 0L,
                        INITIAL_CLOCK_SYNC_INTERVAL_SEC, TimeUnit.SECONDS);
                notifyStatusChange(getText(R.string.bt_service_sync_clocks_status));
                while (true) {
                    try {
                        String msg = waitForMessage();
                        try {
                            JSONObject json = (JSONObject) new JSONTokener(msg).nextValue();
                            processMessage(json);
                        } catch (Exception e) {
                            Log.e(TAG, "failed to process [" + msg + "]: " + e.getMessage(), e);
                        }
                    } catch (IOException e) {
                        Log.e(TAG, e.getMessage(), e);
                        state = State.ERROR;
                        notifyStatusChange(e.getMessage());
                        break;
                    }
                }
                clocksDelta = null;
            }
            synchronized (serviceExecutor) {
                serviceExecutor.wait();
            }
        }
    }

    private String waitForMessage() throws IOException {
        StringBuilder msg = new StringBuilder();
        int b;
        while (true) {
            b = in.read();
            if (b == -1 || b == '\0' || b == '\n') {
                return msg.toString();
            } else {
                if (b == '\r') {
                    // ignore
                } else {
                    msg.append(Character.toChars(b));
                }
            }
        }
    }

    private void processMessage(JSONObject json) throws JSONException, IOException {
        lastMessageTimestamp = new Date();
        ControllerConfig.MessageType msgType = ControllerConfig.MessageType.forCode(json
                .getString(ControllerConfig.MSG_TYPE_KEY));
        if (msgType == ControllerConfig.MessageType.CLOCK_SYNC) {
            long controllerTimestamp = json.getLong(ControllerConfig.TIMESTAMP_KEY);
            int overflowCount = json.getInt(ControllerConfig.OVERFLOW_COUNT_KEY);
            long delta = GregorianCalendar.getInstance().getTimeInMillis()
                    - (controllerTimestamp + overflowCount * (long) Math.pow(2, 32));
            clockSyncExecutor.shutdownNow();
            notifyStatusChange(getText(R.string.bt_service_listening_status));
            if (clocksDelta == null) {
                // initial sync: request for status
                CurrentStatusRequest csr = new CurrentStatusRequest();
                out.write(csr.toJson().toString().getBytes());
            }
            clocksDelta = delta;

        } else {
            if (clocksDelta != null) {
                Intent intent = new Intent();
                intent.setAction(Constants.CONTROLLER_DATA_TRANSFER_ACTION);
                if (msgType == ControllerConfig.MessageType.CURRENT_STATUS_REPORT) {
                    CurrentStatusResponse stats = new CurrentStatusResponse(clocksDelta)
                            .fromJson(json);
                    if (stats != null) {
                        stats.setSrcAddress(dev.getAddress());
                        intent.putExtra(Constants.DATA_KEY, stats);
                        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
                        notifyStatusChange(getText(R.string.bt_service_rceiving_data_status));
                    }
                } else if (msgType == ControllerConfig.MessageType.NODE_STATE_CHANGED) {
                    NodeStateChangeResponse nodeState = new NodeStateChangeResponse(clocksDelta)
                            .fromJson(json);
                    if (nodeState != null) {
                        nodeState.setSrcAddress(dev.getAddress());
                        intent.putExtra(Constants.DATA_KEY, nodeState);
                        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
                        notifyStatusChange(getText(R.string.bt_service_rceiving_data_status));
                    }
                } else {
                    // unknown message
                    Log.w(TAG, "ignore message type:" + msgType);
                }
            } else {
                // ignore any message until clocks not synchronized
                Log.w(TAG, "clocks not synchronized. ignoring message: " + json.toString());
            }
        }
    }

    private void notifyStatusChange(CharSequence details) {
        if (serviceNotification != null) {
            NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            if (dev != null) {
                serviceNotification.setContentText(dev.getName() + ": "
                        + getText(state.toStringResource()));
            } else {
                serviceNotification.setContentText(getText(R.string.unknown_bt_dev));
            }
            manager.notify(CONTROLLER_SERVICE_ONGOING_NOTIFICATION_ID, serviceNotification.build());
        }
        Intent intent = new Intent();
        intent.setAction(Constants.BT_SERVICE_STATUS_ACTION);
        intent.putExtra(Constants.SERVICE_STATUS_KEY, state.toString());
        if (details != null) {
            intent.putExtra(Constants.SERVICE_STATUS_DETAILS_KEY, details);
        }
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    private class SyncClockRequest implements Runnable {
        @Override
        public void run() {
            try {
                ClockSyncRequest req = new ClockSyncRequest();
                out.write(req.toJson().toString().getBytes());
            } catch (Exception e) {
                Log.e(TAG, "failed to sync clocks: " + e.getMessage(), e);
            }
        }
    }

    private class ControllerCommandsReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (CONTROLLER_CONTROL_COMMAND_ACTION.equals(action)) {
                Request command = intent.getParcelableExtra(COMMAND_KEY);
                if (state == State.ACTIVE && out != null && clocksDelta != null) {
                    try {
                        if (command instanceof CurrentStatusRequest) {
                            ((CurrentStatusRequest) command).setClocksDelta(clocksDelta);
                            out.write(command.toJson().toString().getBytes());
                        } else if (command instanceof NodeStateChangeRequest) {
                            out.write(command.toJson().toString().getBytes());
                        } else {
                            Log.w(TAG, "command was ignored: " + command.toString());
                        }
                    } catch (IOException e) {
                        Log.e(TAG,
                                "failed to send command: " + command.toString() + ": "
                                        + e.getMessage(), e);
                    }
                } else {
                    Log.w(TAG,
                            "Service is not initialized. Command was ignored: "
                                    + command.toString());
                }
            }
        }
    }

}

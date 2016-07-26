package com.gagara.homekeeper.service;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Date;
import java.util.UUID;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.json.JSONObject;
import org.json.JSONTokener;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.IntentFilter;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.nbi.request.ClockSyncRequest;
import com.gagara.homekeeper.nbi.request.CurrentStatusRequest;
import com.gagara.homekeeper.nbi.request.NodeStateChangeRequest;
import com.gagara.homekeeper.nbi.request.Request;
import com.gagara.homekeeper.utils.HomeKeeperConfig;

public class BluetoothNbiService extends AbstractNbiService {

    private static final String SP_UUID = "00001101-0000-1000-8000-00805F9B34FB";
    private static final String TAG = BluetoothNbiService.class.getName();

    private static final long HEALTHCKECK_INTERVAL_SEC = 10;

    private BluetoothDevice dev;

    private BluetoothSocket socket = null;
    private InputStream in = null;
    private OutputStream out = null;

    private ScheduledExecutorService healthckeckExecutor = null;

    @Override
    public String getServiceProviderName() {
        if (dev != null) {
            return dev.getName().toString();
        } else {
            return null;
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        healthckeckExecutor = Executors.newSingleThreadScheduledExecutor();
    }

    @Override
    public void setupService() {
        try {
            dev = HomeKeeperConfig.getBluetoothDevice(BluetoothNbiService.this);
            initOngoingNotification();
            if (dev != null) {
                startHealthckecker();
                runService();
            }
        } catch (InterruptedException e) {
            // just terminating
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        healthckeckExecutor.shutdownNow();
        closeConnection();
    }

    @Override
    public void send(Request request) {
        if (state == State.ACTIVE && out != null) {
            try {
                if (request instanceof ClockSyncRequest) {
                    out.write(request.toJson().toString().getBytes());
                } else if (request instanceof CurrentStatusRequest) {
                    out.write(request.toJson().toString().getBytes());
                } else if (request instanceof NodeStateChangeRequest) {
                    out.write(request.toJson().toString().getBytes());
                } else {
                    Log.w(TAG, "command was ignored: " + request.toString());
                }
            } catch (IOException e) {
                Log.e(TAG, "failed to send command: " + request.toString() + ": " + e.getMessage(), e);
            }
        } else {
            Log.w(TAG, "Service is not initialized. Command was ignored: " + request.toString());
        }
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
                clockSyncExecutor.scheduleAtFixedRate(new SyncClockRequest(), 0L, INITIAL_CLOCK_SYNC_INTERVAL_SEC,
                        TimeUnit.SECONDS);
                notifyStatusChange(getText(R.string.service_sync_clocks_status));
                while (true) {
                    try {
                        String msg = waitForMessage();
                        try {
                            JSONObject json = (JSONObject) new JSONTokener(msg).nextValue();
                            processMessage(json, new Date());
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
}

package com.gagara.homekeeper.nbi.service;

import static com.gagara.homekeeper.common.Constants.MESSAGE_KEY;
import static com.gagara.homekeeper.common.Constants.TIMESTAMP_KEY;
import static com.gagara.homekeeper.nbi.service.ServiceState.ACTIVE;
import static com.gagara.homekeeper.nbi.service.ServiceState.ERROR;
import static com.gagara.homekeeper.nbi.service.ServiceState.INIT;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.json.JSONArray;
import org.json.JSONObject;

import android.content.IntentFilter;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import com.gagara.homekeeper.R;
import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.Proxy;
import com.gagara.homekeeper.nbi.request.Request;
import com.gagara.homekeeper.utils.HomeKeeperConfig;

public class ProxyNbiService extends AbstractNbiService {

    private static final String TAG = ProxyNbiService.class.getName();
    
    private static final String PROXY_DATE_FORMAT = "yyyy-MM-ddTHH:mm:ss.SSS'Z'";
    private static DateFormat df = new SimpleDateFormat(PROXY_DATE_FORMAT, Locale.getDefault());

    private Proxy proxy;

    private ScheduledExecutorService logMonitorExecutor = null;

    @Override
    public String getServiceProviderName() {
        if (proxy != null) {
            return proxy.getHost() + ":" + proxy.getPort();
        } else {
            return null;
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        logMonitorExecutor = Executors.newSingleThreadScheduledExecutor();
    }

    @Override
    public void setupService() {
        try {
            proxy = HomeKeeperConfig.getNbiProxy(ProxyNbiService.this);
            initOngoingNotification();
            if (proxy != null) {
                runService();
            }
        } catch (InterruptedException e) {
            // just terminating
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        logMonitorExecutor.shutdownNow();
    }

    @Override
    public void send(Request request) {
        // TODO
    }

    private void runService() throws InterruptedException {
        LocalBroadcastManager.getInstance(this).registerReceiver(
                controllerCommandReceiver,
                new IntentFilter(Constants.CONTROLLER_CONTROL_COMMAND_ACTION));
        lastMessageTimestamp = new Date();
        startLogMonitor();
        while (true) {
            if (state == INIT || state == ERROR) {
                clocksDelta = null;
                clockSyncExecutor = Executors
                        .newSingleThreadScheduledExecutor();
                clockSyncExecutor.scheduleAtFixedRate(new SyncClockRequest(),
                        0L, proxy.getPullPeriod() * 3, TimeUnit.SECONDS);
                notifyStatusChange(getText(R.string.service_sync_clocks_status));
                state = ACTIVE;
            }
            synchronized (serviceExecutor) {
                serviceExecutor.wait();
            }
        }
    }

    private void startLogMonitor() {
        state = INIT;
        notifyStatusChange(null);
        if (!logMonitorExecutor.isTerminated()) {
            logMonitorExecutor.scheduleAtFixedRate(new Runnable() {
                @Override
                public void run() {
                    try {
                        JSONArray logs = requestLogs(lastMessageTimestamp);
                        for (int i = 0; i < logs.length(); i++) {
                            JSONObject log = logs.getJSONObject(i);
                            JSONObject message = log.getJSONObject(MESSAGE_KEY);
                            Date timestamp = df.parse(log.getString(TIMESTAMP_KEY));
                            processMessage(message, timestamp);
                        }
                    } catch (Exception e) {
                        Log.e(TAG, e.getMessage(), e);
                        state = ERROR;
                        synchronized (serviceExecutor) {
                            serviceExecutor.notifyAll();
                        }
                    }
                }
            }, 0L, proxy.getPullPeriod(), TimeUnit.SECONDS);
        }
    }

    private JSONArray requestLogs(Date timestamp) {
        JSONArray result = new JSONArray();
        // TODO
        return result;
    }
}

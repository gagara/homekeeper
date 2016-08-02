package com.gagara.homekeeper.nbi.service;

import static com.android.volley.Request.Method.POST;
import static com.gagara.homekeeper.common.Constants.MESSAGE_KEY;
import static com.gagara.homekeeper.common.Constants.SERVICE_TITLE_CHANGE_ACTION;
import static com.gagara.homekeeper.common.Constants.SERVICE_TITLE_KEY;
import static com.gagara.homekeeper.common.Constants.TIMESTAMP_KEY;
import static com.gagara.homekeeper.nbi.service.ServiceState.ACTIVE;
import static com.gagara.homekeeper.nbi.service.ServiceState.ERROR;
import static com.gagara.homekeeper.nbi.service.ServiceState.INIT;

import java.io.UnsupportedEncodingException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.TimeZone;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import android.content.Intent;
import android.content.IntentFilter;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Base64;
import android.util.Log;

import com.android.volley.AuthFailureError;
import com.android.volley.DefaultRetryPolicy;
import com.android.volley.NetworkResponse;
import com.android.volley.ParseError;
import com.android.volley.RequestQueue;
import com.android.volley.Response;
import com.android.volley.Response.ErrorListener;
import com.android.volley.Response.Listener;
import com.android.volley.VolleyError;
import com.android.volley.toolbox.HttpHeaderParser;
import com.android.volley.toolbox.JsonObjectRequest;
import com.android.volley.toolbox.JsonRequest;
import com.android.volley.toolbox.Volley;
import com.gagara.homekeeper.R;
import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.Proxy;
import com.gagara.homekeeper.nbi.request.LogRequest;
import com.gagara.homekeeper.nbi.request.Request;
import com.gagara.homekeeper.utils.HomeKeeperConfig;

public class ProxyNbiService extends AbstractNbiService {

    private static final String TAG = ProxyNbiService.class.getName();

    private static final int HTTP_CONNECTION_TIMEOUT = 5000;

    private static final String PROXY_DATE_FORMAT = "yyyy-MM-dd'T'HH:mm:ss.SSS'Z'";
    private static final DateFormat df = new SimpleDateFormat(PROXY_DATE_FORMAT, Locale.getDefault());

    private Proxy proxy = null;
    private RequestQueue httpRequestQueue = null;

    private ScheduledExecutorService logMonitorExecutor = null;

    static {
        df.setTimeZone(TimeZone.getTimeZone("UTC"));
    }

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
        proxy = HomeKeeperConfig.getNbiProxy(ProxyNbiService.this);
        logMonitorExecutor = Executors.newSingleThreadScheduledExecutor();
        httpRequestQueue = Volley.newRequestQueue(this);
        httpRequestQueue.start();

        // update title
        Intent intent = new Intent();
        intent.setAction(SERVICE_TITLE_CHANGE_ACTION);
        intent.putExtra(SERVICE_TITLE_KEY, proxy.getHost() + ":" + proxy.getPort());
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    @Override
    public void setupService() {
        try {
            initOngoingNotification();
            runService();
        } catch (InterruptedException e) {
            // just terminating
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        logMonitorExecutor.shutdownNow();
        httpRequestQueue.stop();
    }

    @Override
    public void send(Request request) {
        JsonObjectRequest req = new ProxySendRequest(request);
        req.setRetryPolicy(new DefaultRetryPolicy(HTTP_CONNECTION_TIMEOUT, DefaultRetryPolicy.DEFAULT_MAX_RETRIES,
                DefaultRetryPolicy.DEFAULT_BACKOFF_MULT));
        httpRequestQueue.add(req);
    }

    private void runService() throws InterruptedException {
        LocalBroadcastManager.getInstance(this).registerReceiver(controllerCommandReceiver,
                new IntentFilter(Constants.CONTROLLER_CONTROL_COMMAND_ACTION));

        lastMessageTimestamp = new Date();
        state = INIT;

        startLogMonitor();

        while (true) {
            if (state == INIT || state == ERROR) {
                synchronized (serviceExecutor) {
                    if (clocksDelta == null && clockSyncExecutor == null) {
                        clockSyncExecutor = Executors.newSingleThreadScheduledExecutor();
                        clockSyncExecutor.scheduleAtFixedRate(new SyncClockRequest(), 0L, proxy.getPullPeriod() * 3,
                                TimeUnit.SECONDS);
                    }
                    notifyStatusChange(getText(R.string.service_sync_clocks_status));
                }
                state = ACTIVE;
            }
            synchronized (serviceExecutor) {
                serviceExecutor.wait();
            }
        }
    }

    private void startLogMonitor() {
        if (!logMonitorExecutor.isShutdown()) {
            logMonitorExecutor.scheduleAtFixedRate(new Runnable() {
                @Override
                public void run() {
                    try {
                        JsonRequest<JSONArray> req = new ProxyLogRequest();
                        req.setRetryPolicy(new DefaultRetryPolicy(HTTP_CONNECTION_TIMEOUT,
                                DefaultRetryPolicy.DEFAULT_MAX_RETRIES, DefaultRetryPolicy.DEFAULT_BACKOFF_MULT));
                        httpRequestQueue.add(req);
                    } catch (Exception e) {
                        Log.e(TAG, e.getMessage(), e);
                        state = ERROR;
                        notifyStatusChange(e.getClass().getSimpleName() + ": " + e.getMessage());
                        synchronized (serviceExecutor) {
                            clocksDelta = null;
                            serviceExecutor.notifyAll();
                        }
                    }
                }
            }, 0L, proxy.getPullPeriod(), TimeUnit.SECONDS);
        }
    }

    private Map<String, String> addAuthHeaders(Map<String, String> headers) {
        String creds = String.format("%s:%s", proxy.getUsername(), proxy.getPassword());
        String auth = "Basic " + Base64.encodeToString(creds.getBytes(), Base64.DEFAULT);
        headers.put("Authorization", auth);
        return headers;
    }

    private class ProxySendRequest extends JsonObjectRequest {

        public ProxySendRequest(Request request) {
            super(POST, proxy.asUrl(), request.toJson(), new Response.Listener<JSONObject>() {

                @Override
                public void onResponse(JSONObject response) {
                    // do nothing
                    Log.i(TAG, response.toString());
                }
            }, new Response.ErrorListener() {

                @Override
                public void onErrorResponse(VolleyError e) {
                    Log.e(TAG, e.getMessage(), e);
                    state = ERROR;
                    notifyStatusChange(e.getClass().getSimpleName() + ": " + e.getMessage());
                    synchronized (serviceExecutor) {
                        clocksDelta = null;
                        serviceExecutor.notifyAll();
                    }
                }
            });
        }

        public ProxySendRequest(int method, String url, JSONObject jsonRequest, Listener<JSONObject> listener,
                ErrorListener errorListener) {
            super(method, url, jsonRequest, listener, errorListener);
        }

        @Override
        public Map<String, String> getHeaders() throws AuthFailureError {
            Map<String, String> headers = new HashMap<String, String>(super.getHeaders());
            return addAuthHeaders(headers);
        }
    }

    private class ProxyLogRequest extends JsonRequest<JSONArray> {

        public ProxyLogRequest() {
            super(POST, proxy.asUrl(), new LogRequest(lastMessageTimestamp).toJson().toString(),
                    new Response.Listener<JSONArray>() {

                        @Override
                        public void onResponse(JSONArray response) {
                            for (int i = 0; i < response.length(); i++) {
                                try {
                                    JSONObject log = response.getJSONObject(i);
                                    Date timestamp = df.parse(log.getString(TIMESTAMP_KEY));
                                    lastMessageTimestamp = timestamp;
                                    JSONObject message = (JSONObject) new JSONTokener(log.getString(MESSAGE_KEY))
                                            .nextValue();
                                    processMessage(message, timestamp);
                                } catch (Exception e) {
                                    Log.e(TAG, "failed to process [" + response + "]: " + e.getMessage(), e);
                                }
                            }
                        }
                    }, new Response.ErrorListener() {

                        @Override
                        public void onErrorResponse(VolleyError e) {
                            Log.e(TAG, e.getMessage(), e);
                            state = ERROR;
                            notifyStatusChange(e.getClass().getSimpleName() + ": " + e.getMessage());
                            synchronized (serviceExecutor) {
                                clocksDelta = null;
                                serviceExecutor.notifyAll();
                            }
                        }
                    });
        }

        public ProxyLogRequest(int method, String url, String requestBody, Listener<JSONArray> listener,
                ErrorListener errorListener) {
            super(method, url, requestBody, listener, errorListener);
        }

        @Override
        public Map<String, String> getHeaders() throws AuthFailureError {
            Map<String, String> headers = new HashMap<String, String>(super.getHeaders());
            return addAuthHeaders(headers);
        }

        @Override
        protected Response<JSONArray> parseNetworkResponse(NetworkResponse response) {
            try {
                String jsonString = new String(response.data, HttpHeaderParser.parseCharset(response.headers,
                        PROTOCOL_CHARSET));
                return Response.success(new JSONArray(jsonString), HttpHeaderParser.parseCacheHeaders(response));
            } catch (UnsupportedEncodingException e) {
                return Response.error(new ParseError(e));
            } catch (JSONException je) {
                return Response.error(new ParseError(je));
            }
        }
    }
}

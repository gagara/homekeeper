package com.gagara.homekeeper.nbi.service;

import static com.android.volley.Request.Method.POST;
import static com.gagara.homekeeper.common.Constants.MESSAGE_KEY;
import static com.gagara.homekeeper.common.Constants.TIMESTAMP_KEY;
import static com.gagara.homekeeper.nbi.service.ServiceState.ACTIVE;
import static com.gagara.homekeeper.nbi.service.ServiceState.ERROR;
import static com.gagara.homekeeper.nbi.service.ServiceState.INIT;
import static com.gagara.homekeeper.nbi.service.ServiceState.SHUTDOWN;

import java.io.UnsupportedEncodingException;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
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
import com.android.volley.toolbox.JsonRequest;
import com.android.volley.toolbox.Volley;
import com.gagara.homekeeper.R;
import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.ControllerConfig;
import com.gagara.homekeeper.common.Proxy;
import com.gagara.homekeeper.nbi.https.HttpsTrustManager;
import com.gagara.homekeeper.nbi.request.FastClockSyncRequest;
import com.gagara.homekeeper.nbi.request.LogRequest;
import com.gagara.homekeeper.nbi.request.Request;
import com.gagara.homekeeper.utils.HomeKeeperConfig;

public class ProxyNbiService extends AbstractNbiService {

    private static final String TAG = ProxyNbiService.class.getName();

    private static final int HTTP_CONNECTION_TIMEOUT = 30000; // 30 seconds

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
        try {
            HttpsTrustManager.allowAllSSL();
        } catch (KeyManagementException e) {
            Log.e(TAG, e.getMessage(), e);
        } catch (NoSuchAlgorithmException e) {
            Log.e(TAG, e.getMessage(), e);
        }
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
        if (state != SHUTDOWN) {
            JsonRequest<JSONArray> req = new ProxyRequest(request);
            req.setRetryPolicy(new DefaultRetryPolicy(HTTP_CONNECTION_TIMEOUT, DefaultRetryPolicy.DEFAULT_MAX_RETRIES,
                    DefaultRetryPolicy.DEFAULT_BACKOFF_MULT));
            httpRequestQueue.add(req);
        }
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
                    if (state != ACTIVE) {
                        state = INIT;
                        if (clocksDelta == null && clockSyncExecutor == null) {
                            if (estimatedClocksDelta == null) {
                                // try fast synchronization
                                send(new FastClockSyncRequest());
                            }
                            // schedule normal synchronization
                            clockSyncExecutor = Executors.newSingleThreadScheduledExecutor();
                            clockSyncExecutor.scheduleAtFixedRate(new SyncClockRequest(), 0L,
                                    proxy.getPullPeriod() * 5, TimeUnit.SECONDS);
                        }
                        notifyStatusChange(getText(R.string.service_sync_clocks_status));
                    }
                }
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
                    if (state != SHUTDOWN) {
                        try {
                            JsonRequest<JSONArray> req = new ProxyRequest(new LogRequest(lastMessageTimestamp));
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

    private class ProxyRequest extends JsonRequest<JSONArray> {

        public ProxyRequest(final Request request) {
            super(POST, proxy.asUrl(), request.toJson().toString(), new Response.Listener<JSONArray>() {

                @Override
                public void onResponse(JSONArray response) {
                    for (int i = 0; i < response.length(); i++) {
                        try {
                            JSONObject log = response.getJSONObject(i);
                            Date timestamp = df.parse(log.getString(TIMESTAMP_KEY));
                            JSONObject message = (JSONObject) new JSONTokener(log.getString(MESSAGE_KEY)).nextValue();
                            if (request instanceof FastClockSyncRequest) {
                                ControllerConfig.MessageType msgType = ControllerConfig.MessageType.forCode(message
                                        .getString(ControllerConfig.MSG_TYPE_KEY));
                                if (msgType == ControllerConfig.MessageType.CLOCK_SYNC) {
                                    message.put(ControllerConfig.MSG_TYPE_KEY,
                                            ControllerConfig.MessageType.FAST_CLOCK_SYNC.code());
                                } else {
                                    lastMessageTimestamp = timestamp;
                                }
                            } else {
                                lastMessageTimestamp = timestamp;
                            }
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
                    String msg = e.getMessage();
                    if (msg == null && e.networkResponse != null) {
                        msg = new String(e.networkResponse.data);
                    }
                    notifyStatusChange(e.getClass().getSimpleName() + ": " + msg);
                }
            });
        }

        public ProxyRequest(int method, String url, String requestBody, Listener<JSONArray> listener,
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

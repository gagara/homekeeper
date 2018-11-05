package com.gagara.homekeeper.nbi.service;

import static com.android.volley.Request.Method.POST;
import static com.gagara.homekeeper.common.Constants.MESSAGE_KEY;
import static com.gagara.homekeeper.common.Constants.TIMESTAMP_KEY;
import static com.gagara.homekeeper.nbi.service.ServiceState.ACTIVE;
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
import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.Gateway;
import com.gagara.homekeeper.nbi.https.HttpsTrustManager;
import com.gagara.homekeeper.nbi.request.LogRequest;
import com.gagara.homekeeper.nbi.request.Request;
import com.gagara.homekeeper.utils.HomeKeeperConfig;

public class GatewayNbiService extends AbstractNbiService {

    private static final String TAG = GatewayNbiService.class.getName();

    private static final int HTTP_CONNECTION_TIMEOUT = 30000; // 30 seconds

    private static final String GATEWAY_DATE_FORMAT = "yyyy-MM-dd'T'HH:mm:ss.SSS'Z'";
    private static final DateFormat df = new SimpleDateFormat(GATEWAY_DATE_FORMAT, Locale.getDefault());

    private Gateway gateway = null;
    private RequestQueue httpRequestQueue = null;

    private ScheduledExecutorService logMonitorExecutor = null;

    static {
        df.setTimeZone(TimeZone.getTimeZone("UTC"));
    }

    @Override
    public String getServiceProviderName() {
        if (gateway != null) {
            return gateway.getHost() + ":" + gateway.getPort();
        } else {
            return null;
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        gateway = HomeKeeperConfig.getNbiGateway(GatewayNbiService.this);
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
            JsonRequest<JSONArray> req = new GatewayRequest(request);
            req.setRetryPolicy(new DefaultRetryPolicy(HTTP_CONNECTION_TIMEOUT, DefaultRetryPolicy.DEFAULT_MAX_RETRIES,
                    DefaultRetryPolicy.DEFAULT_BACKOFF_MULT));
            httpRequestQueue.add(req);
        }
    }

    private void runService() throws InterruptedException {
        LocalBroadcastManager.getInstance(this).registerReceiver(controllerCommandReceiver,
                new IntentFilter(Constants.CONTROLLER_CONTROL_COMMAND_ACTION));

        lastMessageTimestamp = new Date();
        startLogMonitor();
        state = ACTIVE;
    }

    private void startLogMonitor() {
        if (!logMonitorExecutor.isShutdown()) {
            logMonitorExecutor.scheduleAtFixedRate(new Runnable() {
                @Override
                public void run() {
                    if (state != SHUTDOWN) {
                        try {
                            JsonRequest<JSONArray> req = new GatewayRequest(new LogRequest(lastMessageTimestamp));
                            req.setRetryPolicy(new DefaultRetryPolicy(HTTP_CONNECTION_TIMEOUT,
                                    DefaultRetryPolicy.DEFAULT_MAX_RETRIES, DefaultRetryPolicy.DEFAULT_BACKOFF_MULT));
                            httpRequestQueue.add(req);
                        } catch (Exception e) {
                            Log.e(TAG, e.getMessage(), e);
                            notifyStatusChange(e.getClass().getSimpleName() + ": " + e.getMessage());
                            synchronized (serviceExecutor) {
                                serviceExecutor.notifyAll();
                            }
                        }
                    }
                }
            }, 0L, gateway.getPullPeriod(), TimeUnit.SECONDS);
        }
    }

    private Map<String, String> addAuthHeaders(Map<String, String> headers) {
        String creds = String.format("%s:%s", gateway.getUsername(), gateway.getPassword());
        String auth = "Basic " + Base64.encodeToString(creds.getBytes(), Base64.DEFAULT);
        headers.put("Authorization", auth);
        return headers;
    }

    private class GatewayRequest extends JsonRequest<JSONArray> {

        public GatewayRequest(final Request request) {
            super(POST, gateway.asUrl(), request.toJson().toString(), new Response.Listener<JSONArray>() {

                @Override
                public void onResponse(JSONArray response) {
                    for (int i = 0; i < response.length(); i++) {
                        try {
                            JSONObject log = response.getJSONObject(i);
                            Date timestamp = df.parse(log.getString(TIMESTAMP_KEY));
                            JSONObject message = (JSONObject) new JSONTokener(log.getString(MESSAGE_KEY)).nextValue();
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

        public GatewayRequest(int method, String url, String requestBody, Listener<JSONArray> listener,
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

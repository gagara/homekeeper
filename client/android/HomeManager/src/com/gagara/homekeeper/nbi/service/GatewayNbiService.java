package com.gagara.homekeeper.nbi.service;

import static com.android.volley.Request.Method.POST;
import static com.gagara.homekeeper.common.Constants.MESSAGE_KEY;
import static com.gagara.homekeeper.common.Constants.SERVICE_STATUS_CHANGE_ACTION;
import static com.gagara.homekeeper.common.Constants.SERVICE_STATUS_DETAILS_KEY;
import static com.gagara.homekeeper.common.Constants.TIMESTAMP_KEY;

import java.io.UnsupportedEncodingException;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import android.content.Intent;
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
import com.gagara.homekeeper.activity.Main;
import com.gagara.homekeeper.common.Constants;
import com.gagara.homekeeper.common.Gateway;
import com.gagara.homekeeper.nbi.https.HttpsTrustManager;
import com.gagara.homekeeper.nbi.request.LogRequest;
import com.gagara.homekeeper.nbi.request.Request;
import com.gagara.homekeeper.utils.HomeKeeperConfig;

public class GatewayNbiService extends AbstractNbiService {

    private static final String TAG = GatewayNbiService.class.getName();

    private static final int HTTP_CONNECTION_TIMEOUT = 30000; // 30 seconds

    private Gateway config = null;
    private RequestQueue httpRequestQueue = null;
    private volatile Date lastMessageTimestamp = null;

    private ScheduledExecutorService logMonitorExecutor = null;

    public void setConfig(Gateway config) {
        this.config = config;
    }

    @Override
    public void init() {
        super.init();
        if (httpRequestQueue == null) {
            httpRequestQueue = Volley.newRequestQueue(Main.getAppContext());
            httpRequestQueue.start();
        }
    }

    @Override
    public void destroy() {
        super.destroy();
        if (httpRequestQueue != null) {
            httpRequestQueue.stop();
            httpRequestQueue = null;
        }
        lastMessageTimestamp = null;
    }

    @Override
    public boolean start() {
        setConfig(HomeKeeperConfig.getNbiGateway(Main.getAppContext()));
        if (config != null && config.valid()) {
            if (logMonitorExecutor == null || logMonitorExecutor.isTerminated()) {
                if (super.start()) {
                    Log.i(TAG, "starting");
                    if (lastMessageTimestamp == null) {
                        lastMessageTimestamp = new Date(
                                (new Date().getTime() / 1000 - Constants.DEFAULT_REFRESH_PERIOD) * 1000);
                    } else {
                        lastMessageTimestamp = new Date();
                    }
                    logMonitorExecutor = Executors.newSingleThreadScheduledExecutor();
                    startLogMonitor();
                    notifyStatusChange(String.format(
                            Main.getAppContext().getResources().getString(R.string.service_pulling_gateway_status),
                            config.getHost(), config.getPort()));
                    try {
                        HttpsTrustManager.allowAllSSL();
                    } catch (KeyManagementException e) {
                        Log.e(TAG, e.getMessage(), e);
                    } catch (NoSuchAlgorithmException e) {
                        Log.e(TAG, e.getMessage(), e);
                    }
                    return true;
                }
            }
        } else {
            Intent intent = new Intent(SERVICE_STATUS_CHANGE_ACTION);
            intent.putExtra(SERVICE_STATUS_DETAILS_KEY,
                    Main.getAppContext().getResources().getString(R.string.gateway_not_configured_error));
            LocalBroadcastManager.getInstance(Main.getAppContext()).sendBroadcast(intent);
            stop();
        }
        return false;
    }

    @Override
    public boolean stop() {
        if (logMonitorExecutor != null) {
            Log.i(TAG, "stopping");
            logMonitorExecutor.shutdownNow();
            logMonitorExecutor = null;
            return super.stop();
        }
        return false;
    }

    @Override
    public boolean pause() {
        return false;
    }

    public void refresh() {
        lastMessageTimestamp = new Date((new Date().getTime() / 1000 - Constants.DEFAULT_REFRESH_PERIOD) * 1000);
    }

    @Override
    public void send(Request request) {
        if (httpRequestQueue != null) {
            JsonRequest<JSONArray> req = new GatewayRequest(request);
            req.setRetryPolicy(new DefaultRetryPolicy(HTTP_CONNECTION_TIMEOUT, DefaultRetryPolicy.DEFAULT_MAX_RETRIES,
                    DefaultRetryPolicy.DEFAULT_BACKOFF_MULT));
            httpRequestQueue.add(req);
        }
    }

    private void startLogMonitor() {
        if (!logMonitorExecutor.isShutdown()) {
            logMonitorExecutor.scheduleAtFixedRate(new Runnable() {
                @Override
                public void run() {
                    try {
                        JsonRequest<JSONArray> req = new GatewayRequest(new LogRequest(lastMessageTimestamp));
                        req.setRetryPolicy(new DefaultRetryPolicy(HTTP_CONNECTION_TIMEOUT,
                                DefaultRetryPolicy.DEFAULT_MAX_RETRIES, DefaultRetryPolicy.DEFAULT_BACKOFF_MULT));
                        httpRequestQueue.add(req);
                        lastMessageTimestamp = new Date();
                    } catch (Exception e) {
                        Log.e(TAG, e.getMessage(), e);
                        notifyStatusChange(e.getClass().getSimpleName() + ": " + e.getMessage());
                    }
                }
            }, 0L, config.getPullPeriod(), TimeUnit.SECONDS);
        }
    }

    private Map<String, String> addAuthHeaders(Map<String, String> headers) {
        String creds = String.format("%s:%s", config.getUsername(), config.getPassword());
        String auth = "Basic " + Base64.encodeToString(creds.getBytes(), Base64.DEFAULT);
        headers.put("Authorization", auth);
        return headers;
    }

    private class GatewayRequest extends JsonRequest<JSONArray> {

        public GatewayRequest(final Request request) {
            super(POST, config.asUrl(), request.toJson().toString(), new Response.Listener<JSONArray>() {

                @Override
                public void onResponse(JSONArray response) {
                    if (response.length() > 0) {
                        for (int i = 0; i < response.length(); i++) {
                            try {
                                JSONObject log = response.getJSONObject(i);
                                Date timestamp = new Date(log.getLong(TIMESTAMP_KEY) * 1000);
                                JSONObject message = (JSONObject) new JSONTokener(log.getString(MESSAGE_KEY))
                                        .nextValue();
                                processMessage(message, timestamp);
                            } catch (Exception e) {
                                Log.e(TAG, "failed to process [" + response + "]: " + e.getMessage(), e);
                            }
                        }
                    } else {
                        notifyStatusChange("-");
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

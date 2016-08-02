package com.gagara.homekeeper.model;

import static com.gagara.homekeeper.common.Constants.MAX_STATUS_RECORDS_TO_SHOW;

import java.util.Queue;

import org.apache.commons.collections4.queue.CircularFifoQueue;

import android.os.Bundle;

public class ServiceLogModel implements Model {

    private Queue<ServiceStatusModel> statusQueue = null;

    public ServiceLogModel() {
        statusQueue = new CircularFifoQueue<ServiceStatusModel>(MAX_STATUS_RECORDS_TO_SHOW);
    }

    @Override
    public void saveState(Bundle bundle) {
        saveState(bundle, "");
    }

    @Override
    public void saveState(Bundle bundle, String prefix) {
        int i = 0;
        for (ServiceStatusModel status : statusQueue) {
            status.saveState(bundle, prefix + "service_log_" + i + "_");
            i++;
        }
        bundle.putInt(prefix + "service_log_count", i);
    }

    @Override
    public void restoreState(Bundle bundle) {
        restoreState(bundle, "");
    }

    @Override
    public void restoreState(Bundle bundle, String prefix) {
        int count = bundle.getInt(prefix + "service_log_count");
        for (int i = 0; i < count; i++) {
            ServiceStatusModel status = new ServiceStatusModel();
            status.restoreState(bundle, prefix + "service_log_" + i + "_");
            statusQueue.add(status);
        }
    }

    @Override
    public boolean isInitialized() {
        return statusQueue != null;
    }

    public void add(ServiceStatusModel status) {
        statusQueue.add(status);
    }

    public Queue<ServiceStatusModel> getLogs() {
        return statusQueue;
    }

}

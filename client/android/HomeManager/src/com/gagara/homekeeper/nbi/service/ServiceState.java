package com.gagara.homekeeper.nbi.service;

import com.gagara.homekeeper.R;

public enum ServiceState {
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

    public static ServiceState fromString(String str) {
        for (ServiceState s : values()) {
            if (s.toString().equalsIgnoreCase(str)) {
                return s;
            }
        }
        return null;
    }
}

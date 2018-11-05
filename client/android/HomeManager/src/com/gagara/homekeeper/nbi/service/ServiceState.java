package com.gagara.homekeeper.nbi.service;

public enum ServiceState {
    INIT, ACTIVE, ERROR, SHUTDOWN;

    public static ServiceState fromString(String str) {
        for (ServiceState s : values()) {
            if (s.toString().equalsIgnoreCase(str)) {
                return s;
            }
        }
        return null;
    }
}

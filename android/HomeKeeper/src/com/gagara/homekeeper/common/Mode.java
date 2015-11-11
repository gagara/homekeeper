package com.gagara.homekeeper.common;

public enum Mode {

    MASTER, SLAVE;

    public static Mode fromString(String s) {
        for (Mode m : Mode.values()) {
            if (m.toString().equalsIgnoreCase(s)) {
                return m;
            }
        }
        return null;
    }

}

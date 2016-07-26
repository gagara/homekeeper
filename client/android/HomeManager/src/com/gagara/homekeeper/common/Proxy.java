package com.gagara.homekeeper.common;

public class Proxy {
    private String host;
    private int port;
    private String username;
    private String password;
    private int pullPeriod;

    public Proxy() {
    }

    public Proxy(String host, int port, String username, String password, int pullPeriod) {
        super();
        this.host = host;
        this.port = port;
        this.username = username;
        this.password = password;
        this.pullPeriod = pullPeriod;
    }

    public String getHost() {
        return host;
    }

    public void setHost(String host) {
        this.host = host;
    }

    public int getPort() {
        return port;
    }

    public void setPort(int port) {
        this.port = port;
    }

    public String getUsername() {
        return username;
    }

    public void setUsername(String username) {
        this.username = username;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword(String password) {
        this.password = password;
    }

    public int getPullPeriod() {
        return pullPeriod;
    }

    public void setPullPeriod(int pullPeriod) {
        this.pullPeriod = pullPeriod;
    }

    @Override
    public String toString() {
        return "Proxy [host=" + host + ", port=" + port + ", username=" + username + ", pullPeriod=" + pullPeriod + "]";
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((host == null) ? 0 : host.hashCode());
        result = prime * result + ((password == null) ? 0 : password.hashCode());
        result = prime * result + port;
        result = prime * result + pullPeriod;
        result = prime * result + ((username == null) ? 0 : username.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        Proxy other = (Proxy) obj;
        if (host == null) {
            if (other.host != null)
                return false;
        } else if (!host.equals(other.host))
            return false;
        if (password == null) {
            if (other.password != null)
                return false;
        } else if (!password.equals(other.password))
            return false;
        if (port != other.port)
            return false;
        if (pullPeriod != other.pullPeriod)
            return false;
        if (username == null) {
            if (other.username != null)
                return false;
        } else if (!username.equals(other.username))
            return false;
        return true;
    }
}

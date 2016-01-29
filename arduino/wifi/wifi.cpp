#include "wifi.h"

#include <SoftwareSerial.h>

static const char WIFI_AP[] = "point39";
static const char WIFI_PW[] = "87654321";

static char OK[] = "OK";

static const uint8_t JSON_MAX_READ_SIZE = 128;

uint8_t clientId = 0;

SoftwareSerial wifi(2, 3); //RX TX

void setup() {
    Serial.begin(115200);
    wifi.begin(115200);
    wifi.setTimeout(300);
    if (wifiConnect()) {
        Serial.println("connected");
    } else {
        Serial.println("not connected");
    }
    if (wifiStartServer()) {
        Serial.println("started");
    }
}

void loop() {
    if (wifi.available() > 0) {
        char* cmd = wifiRead();
//        Serial.println(cmd);
//        Serial.println(clientId);
        free(cmd);
    }
    if (Serial.available() > 0) {
        wifi.println(Serial.readStringUntil('\0'));
    }
    delay(1000);
}

bool wifiConnect() {
    char s[100];
    wifi.println("AT+RST");
    wifi.println("AT+CIPMODE=0");
    wifi.println("AT+CWMODE=1");
    sprintf(s, "AT+CWJAP=\"%s\",\"%s\"", WIFI_AP, WIFI_PW);
    wifi.println(s);
    delay(5000);
    return isWifiConnected();
}

bool isWifiConnected() {
    char s1[] = "STAIP";
    char s2[] = "0.0.0.0";
    wifi.println("AT+CIFSR");
    if (wifi.find(s1)) {
        if (!wifi.find(s2)) {
            return true;
        }
    }
    return false;
}

bool wifiStartServer() {
    wifi.println("AT+CIPMUX=1");
    if (!wifi.find(OK))
        return false;
    wifi.println("AT+CIPSERVER=1,80");
    return wifi.find(OK);
}

bool wifiSend(const char* msg, bool eoi) {
    wifi.print(msg);
    if (eoi) {
        wifi.print("\r\n");
        return wifi.find("OK");
    } else {
        return true;
    }
}

char* wifiRead() {
    String s = wifi.readStringUntil('\0');
    Serial.println(s);
    uint8_t cidx = s.indexOf("+IPD,");
    if (cidx > 0) {
        clientId = s.substring(cidx + 5, (cidx + 5) + 1).toInt();
        Serial.print("clientId:");
        Serial.println(clientId);
        uint8_t rqidx1 = s.indexOf("GET /");
        uint8_t rqidx2 = s.indexOf(" ", rqidx1 + 5);
        if (rqidx1 > 0 && rqidx2 > 0) {
            String rq = s.substring(rqidx1 + 5, rqidx2);
            Serial.print("rq: ");
            Serial.println(rq);
            char* result = (char*) malloc(JSON_MAX_READ_SIZE);
            rq.toCharArray(result, JSON_MAX_READ_SIZE, 0);
            return result;
        }
    }
    return NULL;
}

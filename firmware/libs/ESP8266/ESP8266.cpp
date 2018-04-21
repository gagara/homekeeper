/*
 * ESP8266 to Arduino
 *
 */

#include "ESP8266.h"
#include <debug.h>

//#define __DEBUG__

const uint8_t MAX_AT_REQUEST_SIZE = 64;
const uint8_t MAX_AT_RESPONSE_SIZE = 64;
const uint8_t MAX_MESSAGE_SIZE = 128;
const uint16_t FAILURE_GRACE_PERIOD_SEC = 60;
const uint8_t FAILURE_RECONNECTS_MAX_COUNT = 3;
const uint8_t TCP_CONNECT_RETRY_COUNT = 1;

const char* esp_response_str[] { "\r\nOK\r\n", "\r\nSEND OK\r\n", "CONNECT\r\n", "\r\nWIFI CONNECTED\r\n", "\r\n>",
        "\r\nERROR\r\n" };

#ifdef __DEBUG__
Stream *defaultDebug = &Serial;
#else
Stream *defaultDebug = NULL;
#endif

void ESP8266::init(Stream *port, esp_cwmode mode, uint8_t resetPin) {
    if (!persistDebug) {
        ESP8266::debug = defaultDebug;
    }
    espSerial = port;
    cwMode = mode;
    hwResetPin = resetPin;
    lastSuccessConnectionTs = millis();
    reconnectCount = 0;

    char atcmd[MAX_AT_REQUEST_SIZE + 1];
    dbg(debug, F("wifi: init\n"));
    if (hwResetPin > 0) {
        // hardware reset
        dbg(debug, F("wifi: hw reset\n"));
        digitalWrite(hwResetPin, LOW);
        delay(500);
        digitalWrite(hwResetPin, HIGH);
        delay(1000);
    }
    write(F("AT+RST"), EXPECT_OK, 300);
    delay(1000);
    write(F("AT+CIPMODE=0"), EXPECT_OK, 300);
    sprintf(atcmd, "AT+CWMODE_CUR=%d", mode);
    write(atcmd, EXPECT_OK, 300);
    write(F("AT+CIPMUX=1"), EXPECT_OK, 300);
}

void ESP8266::setDebug(Stream *port) {
    debug = port;
    persistDebug = true;

}

void ESP8266::startAP(const esp_config_t *ssid, const esp_config_t *password) {
    apSsid = (esp_config_t*) ssid;
    apPassword = (esp_config_t*) password;

    if (ssid) {
        if (cwMode == MODE_AP || cwMode == MODE_STA_AP) {
            char atcmd[MAX_AT_REQUEST_SIZE + 1];
            // SSID, password, channel, enc_type, max_conn, hidden
            sprintf(atcmd, "AT+CWSAP_CUR=\"%s\",\"%s\",%d,%d,%d,%d", (char*) ssid, (char*) password, 3, 2, 4, 0); //FIXME: <= 1
            if (write(atcmd, EXPECT_OK, 1000)) {
                dbgf(debug, F("wifi: %s AP started\n"), ssid);
            } else {
                dbg(debug, F("wifi: AP start failed\n"));
            }
        }
    }
}

void ESP8266::connect(const esp_config_t *ssid, const esp_config_t *password) {
    staSsid = (esp_config_t*) ssid;
    staPassword = (esp_config_t*) password;

    if (ssid) {
        if (cwMode == MODE_STA || cwMode == MODE_STA_AP) {
            char atcmd[MAX_AT_REQUEST_SIZE + 1];
            // SSID, password
            sprintf(atcmd, "AT+CWJAP_CUR=\"%s\",\"%s\"", (char*) ssid, (char*) password);
            if (write(atcmd, EXPECT_WIFI_CONNECTED, 5000)) {
                dbgf(debug, F("wifi: connected to %s\n"), ssid);
            } else {
                dbgf(debug, F("wifi: failed connecting to %s\n"), ssid);
            }
            waitUntilBusy(15000, 5);
        }
    }
}

void ESP8266::disconnect() {
    staSsid = NULL;
    staPassword = NULL;

    if (cwMode == MODE_STA || cwMode == MODE_STA_AP) {
        if (write(F("AT+CWQAP"), EXPECT_OK, 1000)) {
            dbg(debug, F("wifi: disconn from AP\n"));
        } else {
            dbg(debug, F("wifi: disconn from AP failed\n"));
        }
    }
}

void ESP8266::reconnect() {
    esp_config_t *ssid = staSsid;
    esp_config_t *password = staPassword;
    disconnect();
    connect(ssid, password);
    reconnectCount++;
}

void ESP8266::startTcpServer(const uint16_t port) {
    tcpServerPort = port;

    if (port > 0) {
        if (tcpServerUp()) {
            dbgf(debug, F("wifi: TCP server UP on port %d\n"), tcpServerPort);
        } else {
            dbg(debug, F("wifi: TCP server start failed\n"));
        }
    }
}

void ESP8266::stopTcpServer() {
    tcpServerPort = 0;

    if (tcpServerDown()) {
        dbg(debug, F("wifi: TCP server DOWN\n"));
    } else {
        dbg(debug, F("wifi: TCP server stop failed\n"));
    }
}

int ESP8266::readApIp(esp_ip_t ip) {
    int res = 0;
    char atrsp[MAX_AT_RESPONSE_SIZE + 1];
    write(F("AT+CIPAP?"));
    if (readUntil(atrsp, MAX_AT_RESPONSE_SIZE, "+CIPAP:ip:")) {
        read(atrsp, MAX_AT_RESPONSE_SIZE);
        res = sscanf(atrsp, "\"%d.%d.%d.%d\"", (int*) &ip[0], (int*) &ip[1], (int*) &ip[2], (int*) &ip[3]);
        while (read(atrsp, MAX_AT_RESPONSE_SIZE))
            ;
    }
    return res;
}

int ESP8266::readStaIp(esp_ip_t ip) {
    int res = 0;
    char atrsp[MAX_AT_RESPONSE_SIZE + 1];
    write(F("AT+CIPSTA?"));
    if (readUntil(atrsp, MAX_AT_RESPONSE_SIZE, "+CIPSTA:ip:")) {
        read(atrsp, MAX_AT_RESPONSE_SIZE);
        res = sscanf(atrsp, "\"%d.%d.%d.%d\"", (int*) &ip[0], (int*) &ip[1], (int*) &ip[2], (int*) &ip[3]);
        while (read(atrsp, MAX_AT_RESPONSE_SIZE))
            ;
    }
    return res;
}

uint16_t ESP8266::send(const esp_ip_t dstIP, const uint16_t dstPort, const char* message) {
    uint16_t httpRsp = 500;

    tcpServerDown();

    //connect to dstIP:dstPort
    uint8_t i = 0;
    bool connected = false;
    char atcmd[MAX_AT_REQUEST_SIZE + 1];
    sprintf(atcmd, "AT+CIPSTART=0,\"TCP\",\"%d.%d.%d.%d\",%d,0", dstIP[0], dstIP[1], dstIP[2], dstIP[3], dstPort);
    while (i < TCP_CONNECT_RETRY_COUNT && !(connected = write(atcmd, EXPECT_CONNECT))) {
        delay(1000);
        i++;
    }
    if (connected) {
        // send HTTP request
        if (write(F("AT+CIPSENDEX=0,2048"), EXPECT_PROMPT)) {
            char rspBuff[MAX_MESSAGE_SIZE + 1];
            sprintf(atcmd, "Content-Length: %d\r\n", strlen(message));
            write(F("POST / HTTP/1.1"));
            write(F("User-Agent: ESP8266"));
            write(F("Accept: */*"));
            write(F("Content-Type: application/json"));
            write(atcmd);
            write(message);
            write("\\0");
            // handle response
            if (readUntil(rspBuff, MAX_MESSAGE_SIZE, "SEND OK", 5000) && readUntil(rspBuff, MAX_MESSAGE_SIZE, "+IPD,0,")
                    && readUntil(rspBuff, MAX_MESSAGE_SIZE, "HTTP/") && read(rspBuff, MAX_MESSAGE_SIZE)) {
                int d;
                sscanf(&rspBuff[0], "%d.%d %d\r\n", &d, &d, &httpRsp);
                // consume rest of response
                while (read(rspBuff, MAX_MESSAGE_SIZE))
                    ;
            }
        }
        lastSuccessConnectionTs = millis();
        reconnectCount = 0;
    }
//    waitUntilBusy();
    write(F("AT+CIPCLOSE=0"), EXPECT_OK);

    tcpServerUp();

    if (httpRsp == 500) {
        checkConnection();
    }
    return httpRsp;
}

size_t ESP8266::receive(char* message, size_t msize) {
    message[0] = '\0';
    char rspBuff[MAX_MESSAGE_SIZE + 1];
    if (readUntil(rspBuff, MAX_MESSAGE_SIZE, "+IPD,0,", 5000)) {
        if (readUntil(rspBuff, MAX_MESSAGE_SIZE, "HTTP/") && strstr(rspBuff, "POST / ")) {
            if (readUntil(rspBuff, MAX_MESSAGE_SIZE, "\r\n\r\n")) {
                read(message, msize);
                char *end;
                if ((end = strstr(message, "0,CLOSED"))) {
                    end = '\0';
                }
                // consume rest of response
                while (read(rspBuff, MAX_MESSAGE_SIZE))
                    ;
                sendResponse(200, "OK");
            } else {
                sendResponse(400, "BAD REQUEST");
            }
        } else {
            sendResponse(404, "NOT FOUND");
        }
        write(F("AT+CIPCLOSE=0"), EXPECT_OK);
    }
    return strlen(message);
}

int ESP8266::available() {
    return espSerial->available();
}

bool ESP8266::write(const char* message, esp_response expectedResponse, const uint16_t ttl, const uint8_t retryCount) {
    for (uint8_t retry = 0; retry < retryCount; retry++) {
        dbgf(debug, F("wifi:>>%s\n"), message);
        // clear input buffer
        while (espSerial->read() >= 0)
            ;
        espSerial->println(message);
        if (expectedResponse != EXPECT_NOTHING) {
            char buf[MAX_AT_RESPONSE_SIZE + 1];
            if (readUntil(buf, MAX_AT_RESPONSE_SIZE, esp_response_str[expectedResponse], ttl / retryCount)) {
                return true;
            }
            if (strstr(buf, esp_response_str[EXPECT_ERROR])) {
                return false;
            }
        } else {
            return true;
        }
    }
    return false;
}

bool ESP8266::write(const __FlashStringHelper* message, esp_response expectedResponse, const uint16_t ttl, const uint8_t retryCount) {
    char buf[MAX_MESSAGE_SIZE + 1];
    int i = 0;
    buf[i] = '\0';
    PGM_P p = reinterpret_cast<PGM_P>(message);
    char c;
    while((c = pgm_read_byte(p++)) != 0 && i < MAX_MESSAGE_SIZE) {
        buf[i++] = c;
        buf[i] = '\0';
    }
    return write(buf, expectedResponse, ttl, retryCount);
}

size_t ESP8266::read(char* buffer, size_t bsize, const uint16_t ttl) {
    unsigned long start = millis();
    uint8_t i = 0;
    buffer[i] = '\0';
    dbgf(debug, F("wifi:<<["));
    while (millis() >= start && millis() - start < ttl && i < bsize) {
        while (espSerial->available() > 0 && i < bsize) {
            char c = (char) espSerial->read();
            bufAdd(buffer, bsize, i, c);
            dbgf(debug, F("%c"), c);
            i++;
        }
    }
    dbgf(debug, F("]\n"));
    return i;
}

bool ESP8266::readUntil(char *buffer, size_t bsize, const char *target, const uint16_t ttl) {
    unsigned long start = millis();
    uint8_t i = 0;
    buffer[i] = '\0';
    dbgf(debug, F("wifi:<<["));
    while (millis() >= start && millis() - start < ttl && !strstr(buffer, target)) {
        while (espSerial->available() > 0 && !strstr(buffer, target)) {
            char c = (char) espSerial->read();
            bufAdd(buffer, bsize, i, c);
            dbgf(debug, F("%c"), c);
            i++;
        }
    }
    dbgf(debug, F("]\n"));
    return (strstr(buffer, target));
}

bool ESP8266::tcpServerUp() {
    if (tcpServerPort > 0) {
        char atcmd[MAX_AT_REQUEST_SIZE + 1];
        sprintf(atcmd, "AT+CIPSERVER=1,%d", tcpServerPort);
        return write(atcmd, EXPECT_OK, 600, 3);
    } else {
        return true;
    }
}

bool ESP8266::tcpServerDown() {
    if (tcpServerPort > 0) {
        return write(F("AT+CIPSERVER=0"), EXPECT_OK, 600, 3);
    } else {
        return true;
    }
}

void ESP8266::checkConnection() {
    unsigned long tsDelta = millis() - lastSuccessConnectionTs;
    if (tsDelta > 0) {
        if (tsDelta < FAILURE_GRACE_PERIOD_SEC * 1000) {
            // skip
        } else {
            if (reconnectCount < FAILURE_RECONNECTS_MAX_COUNT) {
                // reconnect
                reconnect();
                lastSuccessConnectionTs = millis();
            } else {
                // restart ESP8266
                init(espSerial, cwMode, hwResetPin);
                startAP(apSsid, apPassword);
                connect(staSsid, staPassword);
                startTcpServer(tcpServerPort);
            }
        }
    } else {
        lastSuccessConnectionTs = 0;
    }
}
void ESP8266::sendResponse(uint16_t httpCode, const char *content) {
    char rsp[MAX_AT_REQUEST_SIZE + 1];
    sprintf(rsp, "HTTP/1.0 %d %s", httpCode, content);
    if (write(F("AT+CIPSENDEX=0,2048"), EXPECT_PROMPT)) {
        write(rsp);
        write("\\0", EXPECT_SEND_OK);
    }
}

bool ESP8266::waitUntilBusy(const uint16_t ttl, const uint8_t retryCount) {
    return write(F("AT"), EXPECT_OK, ttl, retryCount);
}

void ESP8266::bufAdd(char *buffer, const size_t bsize, const size_t idx, const char c) {
    if (idx < bsize) {
        buffer[idx] = c;
        buffer[idx + 1] = '\0';

    } else {
        size_t i = 0;
        for (i = 0; i < bsize - 1; i++) {
            buffer[i] = buffer[i + 1];
        }
        buffer[i] = c;
        buffer[i + 1] = '\0';
    }
}

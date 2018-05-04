/*
 * ESP8266 to Arduino
 *
 */

#include "ESP8266.h"
#include <debug.h>

//#define __DEBUG__

const uint8_t FAILURE_RECONNECTS_MAX_COUNT = 3;
const unsigned long STA_RECONNECT_INTERVAL = 60000;

const char* esp_response_str[] { "\r\nOK\r\n", "\r\nSEND OK\r\n", "CONNECT\r\n", "\r\nWIFI CONNECTED\r\n", "\r\n>",
        "\r\nERROR\r\n" };

#ifdef __DEBUG__
Stream *defaultDebug = &Serial;
#else
Stream *defaultDebug = NULL;
#endif

// Buffers
const uint8_t IN_BUFF_SIZE = 64;
const uint8_t OUT_BUFF_SIZE = 64;
const uint8_t TMP_BUFF_SIZE = 64;

char inBuff[IN_BUFF_SIZE + 1];
char outBuff[OUT_BUFF_SIZE + 1];
char tmpBuff[TMP_BUFF_SIZE + 1];

void ESP8266::init(Stream *port, esp_cwmode mode, uint8_t resetPin, uint16_t failureGracePeriodSec) {
    if (!persistDebug) {
        debug = defaultDebug;
    }

    dbg(debug, F(":wifi:init\n"));

    espSerial = port;
    cwMode = mode;
    rstPin = resetPin;
    failureGracePeriod = (unsigned long) failureGracePeriodSec * 1000;
    lastSuccessRequestTs = millis();
    staIp[0] = staIp[1] = staIp[2] = staIp[3] = 0;
    connectTs = 0;
    reconnectCount = 0;

    if (rstPin > 0) {
        // hardware reset
        dbg(debug, F(":wifi:hwReset\n"));
        digitalWrite(rstPin, LOW);
        delay(500);
        digitalWrite(rstPin, HIGH);
        delay(1000);
    }
    write(F("AT+RST\r\n"), EXPECT_OK, 300);
    delay(1000);
    write(F("AT+CIPMODE=0\r\n"), EXPECT_OK, 300);
    snprintf(outBuff, OUT_BUFF_SIZE, "AT+CWMODE_CUR=%d\r\n", mode);
    write(outBuff, EXPECT_OK, 300);
    write(F("AT+CIPMUX=1\r\n"), EXPECT_OK, 300);
    write(F("AT+CIPSERVERMAXCONN=1\r\n"), EXPECT_OK, 300);
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
            // SSID, password, channel, enc_type, max_conn, hidden
            snprintf(outBuff, OUT_BUFF_SIZE, "AT+CWSAP_CUR=\"%s\",\"%s\",%d,%d,%d,%d\r\n", (char*) ssid,
                    (char*) password, 3, 2, 4, 0);
            if (write(outBuff, EXPECT_OK, 1000)) {
                dbgf(debug, F(":wifi:AP:start:OK:%s\n"), ssid);
            } else {
                dbg(debug, F(":wifi:AP:start:FAIL:%s\n"));
            }
        }
    }
}

void ESP8266::connect(const esp_config_t *ssid, const esp_config_t *password) {
    staSsid = (esp_config_t*) ssid;
    staPassword = (esp_config_t*) password;

    if (ssid) {
        if (cwMode == MODE_STA || cwMode == MODE_STA_AP) {
            // SSID, password
            snprintf(outBuff, OUT_BUFF_SIZE, "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n", (char*) ssid, (char*) password);
            if (write(outBuff, EXPECT_WIFI_CONNECTED, 5000)) {
                dbgf(debug, F(":wifi:STA:conn:OK:%s\n"), ssid);
            } else {
                dbgf(debug, F(":wifi:STA:conn:FAIL:%s\n"), ssid);
            }
            waitUntilBusy(15000, 5);
            readStaIp(staIp);
            connectTs = millis();
        }
    }
}

void ESP8266::disconnect() {
    staSsid = NULL;
    staPassword = NULL;

    if (cwMode == MODE_STA || cwMode == MODE_STA_AP) {
        if (write(F("AT+CWQAP\r\n"), EXPECT_OK, 1000)) {
            dbg(debug, F(":wifi:STA:disconn:OK\n"));
        } else {
            dbg(debug, F(":wifi:STA:disconn:FAIL\n"));
        }
        staIp[0] = staIp[1] = staIp[2] = staIp[3] = 0;
    }
}

void ESP8266::reconnect() {
    esp_config_t *ssid = staSsid;
    esp_config_t *password = staPassword;
    uint16_t port = tcpServerPort;
    stopTcpServer();
    disconnect();
    connect(ssid, password);
    startTcpServer(port);
    reconnectCount++;
}

bool ESP8266::getStaIP(esp_ip_t ip) {
    ip[0] = staIp[0];
    ip[1] = staIp[1];
    ip[2] = staIp[2];
    ip[3] = staIp[3];
    return validIP(ip);
}

void ESP8266::startTcpServer(const uint16_t port) {
    tcpServerPort = port;
    if (tcpServerPort > 0) {
        snprintf(outBuff, OUT_BUFF_SIZE, "AT+CIPSERVER=1,%d\r\n", tcpServerPort);
        if (write(outBuff, EXPECT_OK, 600, 3)) {
            write(F("AT+CIPSTO=10\r\n"), EXPECT_OK);
            dbgf(debug, F(":wifi:TCP_SRV:up:%d\n"), tcpServerPort);
        } else {
            dbg(debug, F(":wifi:TCP_SRV:up:FAIL\n"));
        }
    }
}

void ESP8266::stopTcpServer() {
    if (tcpServerPort > 0) {
        if (write(F("AT+CIPSERVER=0\r\n"), EXPECT_OK, 600, 3)) {
            dbg(debug, F(":wifi:TCP_SRV:down:OK\n"));
        } else {
            dbg(debug, F(":wifi:TCP_SRV:down:FAIL\n"));
        }
    }
    tcpServerPort = 0;
}

int16_t ESP8266::send(const esp_ip_t dstIP, const uint16_t dstPort, const char* message) {
    int16_t status = httpSend(dstIP, dstPort, message);
    dbgf(debug, F(":wifi:send:%d:%s\n"), status, message);
    if (status == 0) {
        dropConnection();
    }
    if (status <= 0) {
        errorsRecovery();
    }
    return status;
}

size_t ESP8266::receive(char* message, size_t msize) {
    uint16_t status = httpReceive(message, msize);
    dbgf(debug, F(":wifi:receive:%d:%s\n"), status, message);
    if (status == 0) {
        dropConnection();
    }
    return strlen(message);
}

int ESP8266::available() {
    if (!espSerial) {
        return 0;
    }
    return espSerial->available();
}

bool ESP8266::write(const char* message, esp_response expectedResponse, const uint16_t ttl, const uint8_t retryCount) {
    if (!espSerial) {
        return false;
    }
    for (uint8_t retry = 0; retry < retryCount; retry++) {
        dbgf(debug, F("wifi:>>%s"), message);
        if (expectedResponse != EXPECT_NOTHING) {
            // clear input buffer
            while (espSerial->read() >= 0)
                ;
        }
        espSerial->print(message);
        if (expectedResponse != EXPECT_NOTHING) {
            if (readUntil(tmpBuff, TMP_BUFF_SIZE, esp_response_str[expectedResponse], ttl / retryCount)) {
                return true;
            }
            if (strstr(tmpBuff, esp_response_str[EXPECT_ERROR])) {
                return false;
            }
        } else {
            return true;
        }
    }
    return false;
}

bool ESP8266::write(const __FlashStringHelper* message, esp_response expectedResponse, const uint16_t ttl, const uint8_t retryCount) {
    int i = 0;
    outBuff[i] = '\0';
    PGM_P p = reinterpret_cast<PGM_P>(message);
    char c;
    while((c = pgm_read_byte(p++)) != 0 && i < OUT_BUFF_SIZE) {
        outBuff[i++] = c;
        outBuff[i] = '\0';
    }
    return write(outBuff, expectedResponse, ttl, retryCount);
}

size_t ESP8266::read(char* buffer, size_t bsize, const uint16_t ttl) {
    return read(buffer, bsize, NULL, 0, ttl);
}

bool ESP8266::readUntil(char *buffer, size_t bsize, const char *target, const uint16_t ttl) {
    read(buffer, bsize, target, 0, ttl);
    return strstr(buffer, target);
}

bool ESP8266::readUntil(char *buffer, size_t bsize, const __FlashStringHelper *target, const uint16_t ttl) {
    int i = 0;
    tmpBuff[i] = '\0';
    PGM_P p = reinterpret_cast<PGM_P>(target);
    char c;
    while ((c = pgm_read_byte(p++)) != 0 && i < TMP_BUFF_SIZE) {
        tmpBuff[i++] = c;
        tmpBuff[i] = '\0';
    }
    return readUntil(buffer, bsize, tmpBuff, ttl);
}

size_t ESP8266::readUntil(char *buffer, const size_t bsize, const size_t length, const uint16_t ttl) {
    return read(buffer, bsize, NULL, length, ttl);
}

/* ================================== Private ================================== */

int ESP8266::readApIp(esp_ip_t ip) {
    int res = 0;
    write(F("AT+CIPAP?\r\n"));
    if (readUntil(inBuff, IN_BUFF_SIZE, F("+CIPAP:ip:"))) {
        read(inBuff, IN_BUFF_SIZE);
        res = sscanf(inBuff, "\"%d.%d.%d.%d\"", (int*) &ip[0], (int*) &ip[1], (int*) &ip[2], (int*) &ip[3]);
        while (read(inBuff, IN_BUFF_SIZE))
            ;
    }
    return res;
}

int ESP8266::readStaIp(esp_ip_t ip) {
    int res = 0;
    write(F("AT+CIPSTA?\r\n"));
    if (readUntil(inBuff, IN_BUFF_SIZE, F("+CIPSTA:ip:"))) {
        read(inBuff, IN_BUFF_SIZE);
        res = sscanf(inBuff, "\"%d.%d.%d.%d\"", (int*) &ip[0], (int*) &ip[1], (int*) &ip[2], (int*) &ip[3]);
        while (read(inBuff, IN_BUFF_SIZE))
            ;
    }
    return res;
}

size_t ESP8266::read(char *buffer, size_t bsize, const char *target, const size_t length, const uint16_t ttl) {
    if (!espSerial) {
        return 0;
    }
    unsigned long start = millis();
    uint16_t maxRead = length > 0 ? length : bsize;
    uint16_t i = 0;
    buffer[i] = '\0';
    dbgf(debug, F("wifi:<<["));
    while (millis() - start < ttl && ((target != NULL && !strstr(buffer, target)) || (target == NULL && i < maxRead))) {
        while (espSerial->available() > 0
                && ((target != NULL && !strstr(buffer, target)) || (target == NULL && i < maxRead))) {
            char c = (char) espSerial->read();
            append(buffer, bsize, i, c);
            dbgf(debug, F("%c"), c);
            i++;
        }
    }
    dbgf(debug, F("]\n"));
    return i;
}

int16_t ESP8266::httpSend(const esp_ip_t dstIP, const uint16_t dstPort, const char* message) {
    int16_t status = -1;
    if (cwMode == MODE_STA || cwMode == MODE_STA_AP) {
        if (validIP(staIp)) {
            //connect to dstIP:dstPort
            bool connected = false;
            snprintf(outBuff, OUT_BUFF_SIZE, "AT+CIPSTART=4,\"TCP\",\"%d.%d.%d.%d\",%d,0\r\n", dstIP[0], dstIP[1],
                    dstIP[2], dstIP[3], dstPort);
            write(outBuff);
            inBuff[0] = '\0';
            while (readUntil(inBuff, IN_BUFF_SIZE, F("\r\n"), 3000) && strlen(inBuff) > 0) {
                if (strstr(inBuff, "4,CONNECT") || strstr(inBuff, "ALREADY CONNECTED")) {
                    connected = true;
                    break;
                } else if (strstr(inBuff, ",CONNECT") || strstr(inBuff, "ERROR")) {
                    break;
                } else if (strstr(inBuff, "no ip")) {
                    staIp[0] = staIp[1] = staIp[2] = staIp[3] = 0;
                }
            }
            if (connected) {
                status = 0;
                // connection established. send HTTP request
                if (write(F("AT+CIPSENDEX=4,2048\r\n"), EXPECT_PROMPT)) {
                    write(F("POST / HTTP/1.1\r\n"));
                    write(F("User-Agent: ESP8266\r\n"));
                    write(F("Accept: */*\r\n"));
                    write(F("Content-Type: application/json\r\n"));
                    snprintf(outBuff, OUT_BUFF_SIZE, "Content-Length: %d\r\n", strlen(message));
                    write(outBuff);
                    write(F("Connection: close\r\n\r\n"));
                    write(message);
                    write("\\0");
                    // handle response
                    if (readUntil(inBuff, IN_BUFF_SIZE, F("SEND OK"))
                            && readUntil(inBuff, IN_BUFF_SIZE, F("+IPD,4,"), 5000)
                            && readUntil(inBuff, IN_BUFF_SIZE, F("HTTP/"))
                            && readUntil(inBuff, IN_BUFF_SIZE, F("\r\n"))) {
                        int d;
                        if (sscanf(&inBuff[0], "%d.%d %u\r\n", &d, &d, &status)) {
                            // HTTP response code parsed
                            lastSuccessRequestTs = millis();
                            reconnectCount = 0;
                            // consume response body if any
                            if (readUntil(inBuff, IN_BUFF_SIZE, F("+IPD,4,"))
                                    && readUntil(inBuff, IN_BUFF_SIZE, F(":"))) {
                                if (sscanf(&inBuff[0], "%d", &d)) { // body length
                                    readUntil(inBuff, IN_BUFF_SIZE, d);
                                }
                            }
                        }
                    }
                }
                // close connection if required
                if (!strstr(inBuff, "4,CLOSED") && !readUntil(inBuff, IN_BUFF_SIZE, F("4,CLOSED"))) {
                    write(F("AT+CIPCLOSE=4\r\n"), EXPECT_OK);
                }
            }
        }
    }
    return status;
}

int16_t ESP8266::httpReceive(char* message, size_t msize) {
    int16_t status = -1;
    message[0] = '\0';
    if (readUntil(inBuff, IN_BUFF_SIZE, F("+IPD,")) && readUntil(inBuff, IN_BUFF_SIZE, F(":"))) {
        // connection from client established
        status = 0;
        int connId;
        int len;
        bool closed = false;
        if (sscanf(&inBuff[0], "%d,%d:", &connId, &len)) {
            // got connId and request length
            if (readUntil(inBuff, IN_BUFF_SIZE, F("HTTP/")) && strstr(inBuff, "POST / ")) {
                if (readUntil(inBuff, IN_BUFF_SIZE, F("\r\n\r\n"))) { // skip headers
                    read(message, msize);
                    snprintf(tmpBuff, TMP_BUFF_SIZE, "%d,CLOSED", connId);
                    if (strstr(message, tmpBuff)) {
                        // client already closed connection
                        message[strstr(message, tmpBuff) - message] = '\0';
                        closed = true;
                    } else {
                        // consume rest of response until connection closed or timeout
                        while (!readUntil(inBuff, IN_BUFF_SIZE, tmpBuff) && strlen(inBuff) > 0) {
                            if (strstr(inBuff, tmpBuff)) {
                                closed = true;
                            }
                        }
                        if (!closed) {
                            sendResponse(status = 200, "OK");
                        }
                    }
                } else {
                    sendResponse(status = 400, "BAD REQUEST");
                }
            } else {
                sendResponse(status = 404, "NOT FOUND");
            }
            if (!closed) {
                snprintf(outBuff, OUT_BUFF_SIZE, "AT+CIPCLOSE=%d\r\n", connId);
                write(outBuff, EXPECT_OK);
            }
        } else {
            // connId unknown, don't try to close
        }
    }
    return status;
}

void ESP8266::sendResponse(uint16_t httpCode, const char *content) {
    if (write(F("AT+CIPSENDEX=0,2048\r\n"), EXPECT_PROMPT)) {
        snprintf(outBuff, OUT_BUFF_SIZE, "HTTP/1.0 %d %s\r\n", httpCode, content);
        write(outBuff);
        write(F("\\0"), EXPECT_SEND_OK);
    }
}

void ESP8266::errorsRecovery() {
    if (cwMode == MODE_STA || cwMode == MODE_STA_AP) {
        dbg(debug, F(":wifi:ERR_RECOVERY\n"));
        unsigned long ts = millis();
        if ((!validIP(staIp) && (ts - connectTs) > STA_RECONNECT_INTERVAL)
                || (failureGracePeriod > 0 && (ts - lastSuccessRequestTs) > failureGracePeriod)) {
            if (reconnectCount < FAILURE_RECONNECTS_MAX_COUNT) {
                reconnect();
                lastSuccessRequestTs = millis();
            } else {
                // restart ESP8266
                init(espSerial, cwMode, rstPin, failureGracePeriod);
                startAP(apSsid, apPassword);
                connect(staSsid, staPassword);
                startTcpServer(tcpServerPort);
            }
        }
    }
}

void ESP8266::dropConnection() {
    int connId;
    dbg(debug, F(":wifi:DROP_CONN\n"));
    write(F("AT+CIPSTATUS\r\n"));
    while (readUntil(inBuff, IN_BUFF_SIZE, F("\r\n"))) {
        if (strstr(inBuff, "+CIPSTATUS:")) {
            if (sscanf(strstr(inBuff, ":"), ":%d,", &connId)) {
                snprintf(outBuff, OUT_BUFF_SIZE, "AT+CIPCLOSE=%d\r\n", connId);
                write(outBuff, EXPECT_OK);
            }
        } else if (strstr(inBuff, "OK") || strstr(inBuff, "ERROR")) {
            break;
        }
    }
}

bool ESP8266::waitUntilBusy(const uint16_t ttl, const uint8_t retryCount) {
    return write(F("AT\r\n"), EXPECT_OK, ttl, retryCount);
}

void ESP8266::append(char *buffer, const size_t bsize, const size_t idx, const char c) {
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

bool ESP8266::validIP(esp_ip_t ip) {
    return (ip[0] + ip[1] + ip[2] + ip[3]) > 0;
}

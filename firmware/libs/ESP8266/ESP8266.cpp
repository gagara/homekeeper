/*
 * ESP8266 to Arduino
 *
 */

#include "ESP8266.h"
#include <debug.h>

const uint8_t MAX_AT_REQUEST_SIZE = 64;
const uint8_t MAX_AT_RESPONSE_SIZE = 64;
const uint8_t MAX_MESSAGE_SIZE = 128;
const uint16_t SOFT_FAILURE_GRACE_PERIOD_SEC = 60;
const uint16_t HARD_FAILURE_GRACE_PERIOD_SEC = 180;

const char* esp_response_str[] { "\r\nOK\r\n", "\r\nSEND OK\r\n", "\r\nERROR\r\n", "\r\n>", "\r\nWIFI CONNECTED\r\n" };

void ESP8266::init(Stream *espSerial, esp_cwmode mode, uint8_t hwResetPin, Stream *debugSerial) {
    ESP8266::espSerial = espSerial;
    ESP8266::debugSerial = debugSerial;
    ESP8266::mode = mode;
    ESP8266::hwResetPin = hwResetPin;
    ESP8266::lastSuccessCommunicationTs = millis();

    char atcmd[MAX_AT_REQUEST_SIZE + 1];
    dbg(debugSerial, F("wifi: init\n"));
    if (hwResetPin > 0) {
        // hardware reset
        dbg(debugSerial, F("wifi: hw reset\n"));
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

void ESP8266::setDebugPort(Stream *debugSerial) {
    ESP8266::debugSerial = debugSerial;
}

void ESP8266::startAP(const esp_config_t *ssid, const esp_config_t *password) {
    apSsid = (esp_config_t*) ssid;
    apPassword = (esp_config_t*) password;

    if (ssid) {
        if (mode == MODE_AP || mode == MODE_STA_AP) {
            char atcmd[MAX_AT_REQUEST_SIZE + 1];
            // SSID, password, channel, enc_type, conn_cnt, hidden
            sprintf(atcmd, "AT+CWSAP_CUR=\"%s\",\"%s\",%d,%d,%d,%d", (char*) ssid, (char*) password, 3, 2, 1, 1);
            if (write(atcmd, EXPECT_OK, 1000)) {
                dbgf(debugSerial, F("wifi: %s AP started\n"), ssid);
            } else {
                dbg(debugSerial, F("wifi: AP start failed\n"));
            }
        }
    }
}

void ESP8266::connect(const esp_config_t *ssid, const esp_config_t *password) {
    staSsid = (esp_config_t*) ssid;
    staPassword = (esp_config_t*) password;

    if (ssid) {
        if (mode == MODE_STA || mode == MODE_STA_AP) {
            char atcmd[MAX_AT_REQUEST_SIZE + 1];
            // SSID, password
            sprintf(atcmd, "AT+CWJAP_CUR=\"%s\",\"%s\"", (char*) ssid, (char*) password);
            if (write(atcmd, EXPECT_CONNECTED, 5000)) {
                dbgf(debugSerial, F("wifi: connected to %s\n"), ssid);
                delay(2000);
            } else {
                dbg(debugSerial, F("wifi: connection failed\n"));
                write(F("AT"), EXPECT_OK, 15000, 5);
            }
        }
    }
}

void ESP8266::disconnect() {
    staSsid = NULL;
    staPassword = NULL;

    if (mode == MODE_STA || mode == MODE_STA_AP) {
        if (write(F("AT+CWQAP"), EXPECT_OK, 1000)) {
            dbg(debugSerial, F("wifi: disconnected from AP\n"));
        } else {
            dbg(debugSerial, F("wifi: disconnection from AP failed\n"));
        }
    }
}

void ESP8266::reconnect() {
    esp_config_t *ssid = staSsid;
    esp_config_t *password = staPassword;
    disconnect();
    connect(ssid, password);
}

void ESP8266::startTcpServer(const uint16_t port) {
    tcpServerPort = port;

    if (port > 0) {
        if (tcpServerUp()) {
            dbgf(debugSerial, F("wifi: TCP server UP on port %d\n"), tcpServerPort);
        } else {
            dbg(debugSerial, F("wifi: TCP server start failed\n"));
        }
    }
}

void ESP8266::stopTcpServer() {
    tcpServerPort = 0;

    if (tcpServerDown()) {
        dbg(debugSerial, F("wifi: TCP server DOWN\n"));
    } else {
        dbg(debugSerial, F("wifi: TCP server stop failed\n"));
    }
}

int ESP8266::readApIp(esp_ip_t *ip) {
    char atrsp[MAX_AT_RESPONSE_SIZE + 1];
    write(F("AT+CIPAP?"));
    read(atrsp, MAX_AT_RESPONSE_SIZE);
    return sscanf(strstr(atrsp, "+CIPAP:ip:"), "+CIPAP:ip:\"%d.%d.%d.%d\"", (int*) &ip[0], (int*) &ip[1], (int*) &ip[2],
            (int*) &ip[3]);
}

int ESP8266::readStaIp(esp_ip_t *ip) {
    char atrsp[MAX_AT_RESPONSE_SIZE + 1];
    write(F("AT+CIPSTA?"));
    read(atrsp, MAX_AT_RESPONSE_SIZE);
    return sscanf(strstr(atrsp, "+CIPSTA:ip:"), "+CIPSTA:ip:\"%d.%d.%d.%d\"", (int*) ip[0], (int*) ip[1], (int*) ip[2],
            (int*) ip[3]);
}

uint16_t ESP8266::send(const esp_ip_t dstIP, const uint16_t dstPort, const char* message) {
    uint16_t httpRsp = 0;
    unsigned long tsDelta = millis() - lastSuccessCommunicationTs;
    if (tsDelta > 0) {
        if (tsDelta < SOFT_FAILURE_GRACE_PERIOD_SEC) {
            // all ok
        } else if (tsDelta < HARD_FAILURE_GRACE_PERIOD_SEC) {
            reconnect();
        } else {
            init(espSerial, mode, hwResetPin, debugSerial);
            startAP(apSsid, apPassword);
            connect(staSsid, staPassword);
            startTcpServer(tcpServerPort);
        }
    } else {
        lastSuccessCommunicationTs = 0;
    }

    tcpServerDown();

    //connect to dstIP:dstPort
    uint8_t i = 0;
    bool connected = false;
    char atcmd[MAX_AT_REQUEST_SIZE + 1];
    sprintf(atcmd, "AT+CIPSTART=\"TCP\",\"%d.%d.%d.%d\",%d,0", dstIP[0], dstIP[1], dstIP[2], dstIP[3], dstPort);
    while (!(connected = write(atcmd, EXPECT_OK)) && i < 3) {
        delay(1000);
        i++;
    }
    if (connected) {
        // send HTTP request
        if (write(F("AT+CIPSENDEX=0,2048"), EXPECT_PROMPT)) {
            write(F("POST / HTTP/1.1\r\n"));
            write(F("User-Agent: ESP8266\r\n"));
            write(F("Accept: */*\r\n"));
            write(F("Content-Type: application/json\r\n"));
            sprintf(atcmd, "Content-Length: %d\r\n\r\n", strlen(message));
            write(atcmd);
            write(message);
            write(F("\r\n"));
            if (write(F("\0"), EXPECT_SEND_OK)) {
                // parse response
                int d;
                char rspBuff[MAX_MESSAGE_SIZE];
                while (read(rspBuff, MAX_MESSAGE_SIZE), 5000) {
                    if (httpRsp == 0) {
                        if (sscanf(strstr(rspBuff, "HTTP/"), "HTTP/%d.%d %d\r\n", &d, &d, &httpRsp) != 3) {
                            httpRsp = 0;
                        }
                    }
                }
                if (httpRsp == 0) {
                    httpRsp = 500;
                }
            } else {
                httpRsp = 500;
            }
        } else {
            httpRsp = 500;
        }
        write(F("AT+CIPCLOSE=0"));
    } else {
        httpRsp = 500;
    }

    tcpServerUp();

    if (httpRsp != 500) {
        lastSuccessCommunicationTs = millis();
    }
    return httpRsp;
}

size_t ESP8266::receive(char* message, size_t length) {
    // TODO
    // search for HTTP body
    size_t bodyLen = 0, curLen = 0;
    char buffer[MAX_MESSAGE_SIZE + 1];
    while((curLen = read(buffer, MAX_MESSAGE_SIZE)) && bodyLen < length) {
        if (sscanf(strstr(buffer, "\r\n\r\n"), "\r\n\r\n%s", message + bodyLen)) {
            char *end =  strstr(message, "\r\n");
            if (end) {
                end = '\0';
                break;
            }
            bodyLen += curLen - ((strstr(buffer, "\r\n\r\n") - &buffer[0]) + 2);
        }
    }
    return bodyLen;
}

int ESP8266::available() {
    return espSerial->available();
}

bool ESP8266::write(const char* message, esp_response expectedResponse, const uint16_t ttl, const uint8_t retryCount) {
    for (uint8_t retry = 0; retry < retryCount; retry++) {
        dbgf(debugSerial, F("wifi:>>%s\n"), message);
        // clear input buffer
        while (espSerial->read() >= 0);
        espSerial->println(message);
        if (expectedResponse != EXPECT_NOTHING) {
            char buf[MAX_AT_RESPONSE_SIZE + 1];
            uint8_t i = 0;
            buf[i] = '\0';
            read(buf, MAX_AT_RESPONSE_SIZE, ttl / retryCount);
            if (strstr(buf, esp_response_str[expectedResponse])) {
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

size_t ESP8266::read(char* buffer, size_t length, const uint16_t ttl) {
    unsigned long start = millis();
    uint8_t i = 0;
    buffer[i] = '\0';
    dbgf(debugSerial, F("wifi:<<"));
    while (millis() >= start && millis() - start < ttl && i < length) {
        while (espSerial->available() > 0 && i < length) {
            buffer[i++] = (char) espSerial->read();
            buffer[i] = '\0';
            dbgf(debugSerial, &buffer[i - 1]);
        }
    }
    dbgf(debugSerial, F("\n"));
    return i;
}

bool ESP8266::tcpServerUp() {
    if (tcpServerPort > 0) {
        char atcmd[MAX_AT_REQUEST_SIZE + 1];
        sprintf(atcmd, "AT+CIPSERVER=1,%d", tcpServerPort);
        return write(atcmd, EXPECT_OK, 200);
    } else {
        return true;
    }
}

bool ESP8266::tcpServerDown() {
    if (tcpServerPort > 0) {
        return write(F("AT+CIPSERVER=0"), EXPECT_OK, 200);
    } else {
        return true;
    }
}

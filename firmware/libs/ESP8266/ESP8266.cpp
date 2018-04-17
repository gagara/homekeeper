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

const char* esp_response_str[] { "\r\nOK\r\n", "\r\nERROR\r\n", "\r\n>\r\n", "\r\nWIFI CONNECTED\r\n" };

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

void ESP8266::startAP(const char *ssid, const char *password) {
    strncpy(apSsid, ssid, AP_CREDS_MAX_SIZE);
    apSsid[AP_CREDS_MAX_SIZE - 1] = '\0';
    strncpy(apPassword, password, AP_CREDS_MAX_SIZE);
    apPassword[AP_CREDS_MAX_SIZE - 1] = '\0';

    if (strlen(ssid)) {
        if (mode == MODE_AP || mode == MODE_STA_AP) {
            char atcmd[MAX_AT_REQUEST_SIZE + 1];
            // SSID, password, channel, enc_type, conn_cnt, hidden
            sprintf(atcmd, "AT+CWSAP_CUR=\"%s\",\"%s\",%d,%d,%d,%d", ssid, password, 3, 2, 1, 1);
            if (write(atcmd, EXPECT_OK, 1000)) {
                dbgf(debugSerial, F("wifi: %s AP started\n"), ssid);
            } else {
                dbg(debugSerial, F("wifi: AP start failed\n"));
            }
        }
    }
}

void ESP8266::connect(const char *ssid, const char *password) {
    strncpy(staSsid, ssid, AP_CREDS_MAX_SIZE);
    staSsid[AP_CREDS_MAX_SIZE - 1] = '\0';
    strncpy(staPassword, password, AP_CREDS_MAX_SIZE);
    staPassword[AP_CREDS_MAX_SIZE - 1] = '\0';

    if (strlen(ssid)) {
        if (mode == MODE_STA || mode == MODE_STA_AP) {
            char atcmd[MAX_AT_REQUEST_SIZE + 1];
            // SSID, password
            sprintf(atcmd, "AT+CWJAP_CUR=\"%s\",\"%s\"", ssid, password);
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
    staSsid[0] = '\0';
    staPassword[0] = '\0';

    if (mode == MODE_STA || mode == MODE_STA_AP) {
        if (write(F("AT+CWQAP"), EXPECT_OK, 1000)) {
            dbg(debugSerial, F("wifi: disconnected from AP\n"));
        } else {
            dbg(debugSerial, F("wifi: disconnection from AP failed\n"));
        }
    }
}

void ESP8266::reconnect() {
    char ssid[AP_CREDS_MAX_SIZE];
    strncpy(ssid, staSsid, AP_CREDS_MAX_SIZE);
    char password[AP_CREDS_MAX_SIZE];
    strncpy(password, staPassword, AP_CREDS_MAX_SIZE);
    disconnect();
    connect(ssid, password);
}

void ESP8266::startTcpServer(const unsigned int port) {
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

int ESP8266::readApIp(uint8_t *ip) {
    char atrsp[MAX_AT_RESPONSE_SIZE + 1];
    write(F("AT+CIPAP?"));
    read(atrsp, MAX_AT_RESPONSE_SIZE);
    return sscanf(strstr(atrsp, "+CIPAP:ip:"), "+CIPAP:ip:\"%d.%d.%d.%d\"", (int*) &ip[0], (int*) &ip[1], (int*) &ip[2],
            (int*) &ip[3]);
}

int ESP8266::readStaIp(uint8_t *ip) {
    char atrsp[MAX_AT_RESPONSE_SIZE + 1];
    write(F("AT+CIPSTA?"));
    read(atrsp, MAX_AT_RESPONSE_SIZE);
    return sscanf(strstr(atrsp, "+CIPSTA:ip:"), "+CIPSTA:ip:\"%d.%d.%d.%d\"", (int*) &ip[0], (int*) &ip[1],
            (int*) &ip[2], (int*) &ip[3]);
}

void ESP8266::send(const char* message) {
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
    // TODO
    if (write(message)) {
        dbg(debugSerial, "send OK\n");
        lastSuccessCommunicationTs = millis();
    } else {
        dbg(debugSerial, "send ERR\n");
    }
}

size_t ESP8266::receive(char* message, size_t length) {
    // TODO
    char buffer[MAX_MESSAGE_SIZE + 1];
    return read(buffer, MAX_MESSAGE_SIZE);
}

int ESP8266::available() {
    return espSerial->available();
}

bool ESP8266::write(const char* message, esp_response expected_response, const uint16_t ttl,
        const uint8_t retry_count) {
    for (uint8_t retry = 0; retry < retry_count; retry++) {
        dbgf(debugSerial, F("wifi:>>%s\n"), message);
        while (espSerial->read() >= 0) {
            // clear input buffer
        }
        espSerial->println(message);
        if (expected_response != EXPECT_NOTHING) {
            char buf[MAX_AT_RESPONSE_SIZE + 1];
            uint8_t i = 0;
            buf[i] = '\0';
            read(buf, MAX_AT_RESPONSE_SIZE, ttl / retry_count);
            if (strstr(buf, esp_response_str[expected_response])) {
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

bool ESP8266::write(const __FlashStringHelper* message, esp_response expected_response, const uint16_t ttl, const uint8_t retry_count) {
    char buf[MAX_MESSAGE_SIZE + 1];
    int i = 0;
    buf[i] = '\0';
    PGM_P p = reinterpret_cast<PGM_P>(message);
    char c;
    while((c = pgm_read_byte(p++)) != 0 && i < MAX_MESSAGE_SIZE) {
        buf[i++] = c;
        buf[i] = '\0';
    }
    return write(buf, expected_response, ttl, retry_count);
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

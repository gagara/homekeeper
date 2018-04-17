#ifndef WIFI_H_
#define WIFI_H_

#include <Arduino.h>

#define AP_CREDS_MAX_SIZE 16

enum esp_cwmode {
    MODE_UNKNOWN = 0, MODE_STA = 1, MODE_AP = 2, MODE_STA_AP = 3,
};

enum esp_response {
    EXPECT_NOTHING = -1,
    EXPECT_OK = 0,
    EXPECT_ERROR = 1,
    EXPECT_PROMPT = 2,
    EXPECT_CONNECTED = 3
};

class ESP8266 {
public:
    void init(Stream *espSerial, esp_cwmode mode = MODE_STA, uint8_t hwResetPin = 0, Stream *debugSerial = &Serial);
    void setDebugPort(Stream *debugSerial);
    void startAP(const char *ssid, const char *password);
    void connect(const char *ssid, const char *password);
    void disconnect();
    void reconnect();
    void startTcpServer(const unsigned int port);
    void stopTcpServer();
    int readApIp(uint8_t *ip);
    int readStaIp(uint8_t *ip);

    void send(const char* message);
    size_t receive(char* message, size_t length);
    int available();
    bool write(const char *message, esp_response expected_response = EXPECT_NOTHING, const uint16_t ttl = 3000, const uint8_t retry_count = 1);
    bool write(const __FlashStringHelper *message, esp_response expected_response = EXPECT_NOTHING, const uint16_t ttl = 3000, const uint8_t retry_count = 1);
    size_t read(char *buffer, size_t length, const uint16_t ttl = 3000);

private:
    Stream *espSerial;
    Stream *debugSerial;
    uint8_t hwResetPin;
    esp_cwmode mode;
    char apSsid[AP_CREDS_MAX_SIZE] = "\0";
    char apPassword[AP_CREDS_MAX_SIZE] = "\0";
    char staSsid[AP_CREDS_MAX_SIZE] = "\0";
    char staPassword[AP_CREDS_MAX_SIZE] = "\0";
    uint16_t tcpServerPort = 0;
    unsigned long lastSuccessCommunicationTs = 0;
    bool tcpServerUp();
    bool tcpServerDown();
};

#endif /* WIFI_H_ */

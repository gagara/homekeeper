#ifndef WIFI_H_
#define WIFI_H_

#include <Arduino.h>

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
    static void init(Stream *espSerial, esp_cwmode mode = MODE_STA, uint8_t hwResetPin = 0, Stream *debugSerial = &Serial);
    static void setDebugPort(Stream *debugSerial);
    static void startAP(const char *ssid, const char *password);
    static void connect(const char *ssid, const char *password);
    static void disconnect();
    static void startTcpServer(const unsigned int port);
    static void stopTcpServer();
    static int readApIp(uint8_t *ip);
    static int readStaIp(uint8_t *ip);

    static void send(const char* message);
    static size_t receive(char* message, size_t length);
    static int available();
    static bool write(const char *message, esp_response expected_response = EXPECT_NOTHING, const uint16_t ttl = 3000, const uint8_t retry_count = 1);
    static bool write(const __FlashStringHelper *message, esp_response expected_response = EXPECT_NOTHING, const uint16_t ttl = 3000, const uint8_t retry_count = 1);
    static size_t read(char *buffer, size_t length, const uint16_t ttl = 3000);

private:
    static Stream *espSerial;
    static Stream *debugSerial;
    static uint8_t hwResetPin;
    static uint16_t tcpServerPort;
    static unsigned long lastSuccessCommunicationTs;
    static esp_cwmode getMode();
    static bool tcpServerUp();
    static bool tcpServerDown();
};

#endif /* WIFI_H_ */

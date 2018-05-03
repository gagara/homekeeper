#ifndef WIFI_H_
#define WIFI_H_

#include <Arduino.h>

#define ESP_CONFIG_TYPE_SIZE 32

enum esp_cwmode {
    MODE_UNKNOWN = 0, MODE_STA = 1, MODE_AP = 2, MODE_STA_AP = 3,
};

enum esp_response {
    EXPECT_NOTHING = -1,
    EXPECT_OK = 0,
    EXPECT_SEND_OK = 1,
    EXPECT_CONNECT = 2,
    EXPECT_WIFI_CONNECTED = 3,
    EXPECT_PROMPT = 4,
    EXPECT_ERROR = 5,
};

typedef char esp_config_t[ESP_CONFIG_TYPE_SIZE];

typedef uint8_t esp_ip_t[4];

class ESP8266 {
public:
    void init(Stream *port, esp_cwmode mode = MODE_STA, uint8_t resetPin = 0, uint16_t failureGracePeriodSec = 0); //
    void setDebug(Stream *port); //
    void startAP(const esp_config_t *ssid, const esp_config_t *password); //
    void connect(const esp_config_t *ssid, const esp_config_t *password); //
    void disconnect(); //
    void reconnect(); //
    bool getStaIP(esp_ip_t ip); //
    void startTcpServer(const uint16_t port); //
    void stopTcpServer(); //
    uint16_t send(const esp_ip_t dstIP, const uint16_t dstPort, const char* message); //
    size_t receive(char* message, size_t msize); //
    int available(); //
    bool write(const char *message, esp_response expectedResponse = EXPECT_NOTHING, const uint16_t ttl = 1000,
            const uint8_t retryCount = 1); //
    bool write(const __FlashStringHelper *message, esp_response expectedResponse = EXPECT_NOTHING, const uint16_t ttl =
            1000, const uint8_t retryCount = 1); //
    size_t read(char *buffer, size_t bsize, const uint16_t ttl = 1000); //
    bool readUntil(char *buffer, const size_t bsize, const char *target, const uint16_t ttl = 1000);
    bool readUntil(char *buffer, const size_t bsize, const __FlashStringHelper *target, const uint16_t ttl = 1000);
    size_t readUntil(char *buffer, const size_t bsize, const size_t length, const uint16_t ttl = 1000);

private:
    Stream *debug = NULL;
    Stream *espSerial = NULL;
    uint8_t rstPin = 0;
    esp_cwmode cwMode = MODE_UNKNOWN;
    esp_config_t *apSsid = NULL;
    esp_config_t *apPassword = NULL;
    esp_config_t *staSsid = NULL;
    esp_config_t *staPassword = NULL;
    uint16_t tcpServerPort = 0;
    unsigned long failureGracePeriod = 0;
    unsigned long lastSuccessRequestTs = 0;
    esp_ip_t staIp;
    unsigned long connectTs = 0;
    uint8_t reconnectCount = 0;bool persistDebug = false;
    int readApIp(esp_ip_t ip); //
    int readStaIp(esp_ip_t ip); //
    size_t read(char *buffer, size_t bsize, const char *target, const size_t length, const uint16_t ttl); //
    uint16_t httpSend(const esp_ip_t dstIP, const uint16_t dstPort, const char* message); //
    uint16_t httpReceive(char* message, size_t msize); //
    void sendResponse(uint16_t httpCode, const char *content); //
    void errorsRecovery(); //
    void dropConnection(); //
    bool waitUntilBusy(const uint16_t ttl = 5000, const uint8_t retryCount = 1); //
    void bufAdd(char *buffer, const size_t bsize, const size_t idx, const char c); //
    bool validIP(esp_ip_t ip); //
};

#endif /* WIFI_H_ */

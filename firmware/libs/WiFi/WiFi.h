#ifndef WIFI_H_
#define WIFI_H_

#include "Arduino.h"

 enum esp_cwmode {
    MODE_STA = 1, MODE_AP = 2, MODE_STA_AP = 3,
};

enum esp_response {
    OK = "OK", ERROR = "ERROR"
};

class WiFi {
public:
    static void init(Stream *espSerial, uint8_t hwResetPin = 0, Stream *debugSerial = &Serial);
    static void begin(uint8_t mode = 0);

private:
    static Stream *espSerial;
    static Stream *debugSerial;
    static uint8_t hwResetPin;
    static uint8_t staIP[4];
    static bool sendAT(const char* msg, char* rsp = NULL, const unsigned int ttl = 3000, const uint8_t retryCnt = 1);
};

#endif /* WIFI_H_ */

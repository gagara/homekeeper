/*
 * WiFi.h
 */

#ifndef WIFI_H_
#define WIFI_H_

#include <Arduino.h>

class WiFi {
public:
    static void init(Stream *espSerial, uint8_t hwResetPin = 0, Stream *debugSerial = &Serial);
    static void begin(uint8_t mode = 0);

private:
    static Stream *espSerial;
    static Stream *debugSerial;
    static uint8_t hwResetPin;
    static uint8_t stationIP[4];
};

#endif /* WIFI_H_ */

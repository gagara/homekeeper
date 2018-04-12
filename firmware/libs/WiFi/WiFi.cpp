/*
 * ESP8266 to Arduino
 *
 */

#include "WiFi.h"

#include "debug.h"

Stream *WiFi::espSerial;
Stream *WiFi::debugSerial;
uint8_t WiFi::hwResetPin;
uint8_t WiFi::stationIP[4];


void WiFi::init(Stream *espSerial, uint8_t hwResetPin, Stream *debugSerial) {
    WiFi::espSerial = espSerial;
    WiFi::hwResetPin = hwResetPin;
    WiFi::debugSerial = debugSerial;
    dbg(F("wifi init"));
    if (WiFi::hwResetPin > 0) {
        // hardware reset
        digitalWrite(WiFi::hwResetPin, LOW);
        delay(500);
        digitalWrite(WiFi::hwResetPin, HIGH);
        delay(1000);
    }
    WiFi::stationIP[0] = 0;
    WiFi::stationIP[1] = 0;
    WiFi::stationIP[2] = 0;
    WiFi::stationIP[3] = 0;
}

void WiFi::begin(uint8_t mode) {
    //TODO
}

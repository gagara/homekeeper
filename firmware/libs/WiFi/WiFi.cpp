/*
 * ESP8266 to Arduino
 *
 */

#include "WiFi.h"

#include "debug.h"

Stream *WiFi::espSerial;
Stream *WiFi::debugSerial;
uint8_t WiFi::hwResetPin;
uint8_t WiFi::staIP[4];

void WiFi::init(Stream *espSerial, uint8_t hwResetPin, Stream *debugSerial) {
    WiFi::espSerial = espSerial;
    WiFi::hwResetPin = hwResetPin;
    WiFi::debugSerial = debugSerial;
    dbg(F("wifi: init\n"));
    if (WiFi::hwResetPin > 0) {
        // hardware reset
        dbg(F("wifi: hw reset\n"));
        digitalWrite(WiFi::hwResetPin, LOW);
        delay(500);
        digitalWrite(WiFi::hwResetPin, HIGH);
        delay(1000);
    }
    WiFi::staIP[0] = 0;
    WiFi::staIP[1] = 0;
    WiFi::staIP[2] = 0;
    WiFi::staIP[3] = 0;
}

void WiFi::begin(uint8_t mode) {
    dbg(F("wifi: setup mode: " + mode + "\n"));

}

bool sendAT(const char* msg, char* rsp, const unsigned int ttl, const uint8_t retryCnt) {
    unsigned long start = millis();
    for (uint8_t i = 0; i < retryCnt; i++) {
        WiFi::espSerial->println(msg);
        if (rsp != NULL) {
            unsigned long rts = millis();
            while ((rts - start < ttl) && (millis() - rts < ttl / retryCnt)) {
                delay(10);
                if (WiFi::espSerial->find(rsp)) {
                    return true;
                }
                if (WiFi::espSerial->find(ERROR)) {
                    break;
                }
                if (millis() < start || millis() < rts) {
                    // overflow
                    break;
                }
            }
        } else {
            return true;
        }
    }
    return false;
}

#ifndef JSONER_H_
#define JSONER_H_

#include <Arduino.h>

void jsonifyNodeStatus(const uint8_t id, //
        const uint16_t ns, //
        const unsigned long ts, //
        const uint16_t ff, //
        const unsigned long tsf, //
        char *buffer, //
        const size_t bsize);
void jsonifyNodeStateChange(const uint8_t id, //
        const uint16_t ns, //
        const unsigned long ts, //
        const uint16_t ff, //
        const unsigned long tsf, //
        const uint8_t sensId[], //
        const int16_t sensVal[], //
        const uint8_t sensCnt, //
        char *buffer, //
        const size_t bsize);
void jsonifySensorValue(const uint8_t id, //
        const int8_t value, //
        char *buffer, //
        const size_t bsize);
void jsonifySensorValue(const uint8_t id, //
        const double value, //
        char *buffer, //
        const size_t bsize);
void jsonifySensorValue(const uint8_t id, //
        const int8_t value, //
        const unsigned long ts, //
        char *buffer, //
        const size_t bsize);
void jsonifySensorConfig(const uint8_t id, //
        const __FlashStringHelper *key, //
        const double value, //
        char *buffer, //
        const size_t bsize);
void jsonifySensorConfig(const uint8_t id, //
        const __FlashStringHelper *key, //
        const char *value, //
        char *buffer, //
        const size_t bsize);
void jsonifySensorConfig(const uint8_t id, //
        const __FlashStringHelper *key, //
        const int16_t value, //
        char *buffer, //
        const size_t bsize);
void jsonifyConfig(const __FlashStringHelper *key, //
        const int value, //
        char *buffer, //
        const size_t bsize);
void jsonifyConfig(const __FlashStringHelper *key, //
        const char *value, //
        char *buffer, //
        const size_t bsize);
void jsonifyClockSync(const unsigned long value, //
        char *buffer, //
        const size_t bsize);

#endif /* JSONER_H_ */

#include "jsoner.h"

#include <ArduinoJson.h>

void jsonifyNodeStatus(const uint8_t id, //
        const uint8_t ns, //
        const unsigned long ts, //
        const uint8_t ff, //
        const unsigned long tsf, //
        char *buffer, //
        const size_t bsize) {
    DynamicJsonBuffer jsonBuffer(bsize * 2);
    JsonObject& root = jsonBuffer.createObject();
    root[F("m")] = F("csr");

    JsonObject& node = jsonBuffer.createObject();
    node[F("id")] = id;
    node[F("ns")] = ns;
    node[F("ts")] = ts;
    node[F("ff")] = ff;
    if (ff) {
        node[F("ft")] = tsf;
    }

    root[F("n")] = node;

    root.printTo(buffer, bsize);
}

void jsonifyNodeStateChange(const uint8_t id, //
        const uint8_t ns, //
        const unsigned long ts, //
        const uint8_t ff, //
        const unsigned long tsf, //
        const uint8_t sensId[], //
        const int16_t sensVal[], //
        const uint8_t sensCnt, //
        char *buffer, //
        const size_t bsize) {
    DynamicJsonBuffer jsonBuffer(bsize * 2);
    JsonObject& root = jsonBuffer.createObject();
    root[F("m")] = F("nsc");
    root[F("id")] = id;
    root[F("ns")] = ns;
    root[F("ts")] = ts;
    root[F("ff")] = ff;
    if (ff) {
        root[F("ft")] = tsf;
    }
    JsonArray& sensors = root.createNestedArray(F("s"));
    for (unsigned int i = 0; i < sensCnt; i++) {
        JsonObject& sens = jsonBuffer.createObject();
        sens[F("id")] = sensId[i];
        sens[F("v")] = sensVal[i];
        sensors.add(sens);
    }

    root.printTo(buffer, bsize);
}

void jsonifySensorValue(const uint8_t id, //
        const int8_t value, //
        char *buffer, //
        const size_t bsize) {
    DynamicJsonBuffer jsonBuffer(bsize * 2);
    JsonObject& root = jsonBuffer.createObject();
    root[F("m")] = F("csr");

    JsonObject& sens = jsonBuffer.createObject();
    sens[F("id")] = id;
    sens[F("v")] = value;

    root[F("s")] = sens;

    root.printTo(buffer, bsize);
}

void jsonifySensorConfigCf(const uint8_t id, //
        const double value, //
        char *buffer, //
        const size_t bsize) {
    DynamicJsonBuffer jsonBuffer(bsize * 2);
    JsonObject& root = jsonBuffer.createObject();
    root[F("m")] = F("cfg");

    JsonObject& sens = jsonBuffer.createObject();
    sens[F("id")] = id;
    sens[F("cf")] = value;

    root[F("s")] = sens;

    root.printTo(buffer, bsize);
}

void jsonifyConfig(const __FlashStringHelper *key, //
        const void *value, //
        char *buffer, //
        const size_t bsize) {
    DynamicJsonBuffer jsonBuffer(bsize * 2);
    JsonObject& root = jsonBuffer.createObject();
    root[F("m")] = F("cfg");
    root[key] = value;

    root.printTo(buffer, bsize);
}

void jsonifyClockSync(const unsigned long value, //
        char *buffer, //
        const size_t bsize) {
    DynamicJsonBuffer jsonBuffer(bsize * 2);
    JsonObject& root = jsonBuffer.createObject();
    root[F("m")] = F("cls");
    root[F("ts")] = value;

    root.printTo(buffer, bsize);
}


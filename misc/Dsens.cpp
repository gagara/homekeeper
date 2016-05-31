// Do not remove the include below
#include "Dsens.h"

#include <OneWire.h>

#define ONE_WIRE_BUS 31
#define TEMPERATURE_PRECISION 9

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress dev;

DeviceAddress SENSOR_MIX = {0x28, 0xFF, 0x45, 0x90, 0x23, 0x16, 0x04, 0xC5};

void setup() {
    Serial.begin(9600);

    sensors.begin();
    oneWire.reset_search();
    Serial.println("found devices:");
    while (oneWire.search(dev)) {
        printAddress(dev);
        sensors.setResolution(dev, TEMPERATURE_PRECISION);
    }
    Serial.println("end");
}

void loop() {
    Serial.print("Requesting temperatures...");
    sensors.requestTemperatures();
    Serial.println(sensors.getTempC(SENSOR_MIX));
    Serial.println("DONE");
    delay(1000);
}

void printAddress(DeviceAddress deviceAddress) {
    for (uint8_t i = 0; i < 8; i++) {
        // zero pad the address if necessary
        if (deviceAddress[i] < 16)
            Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
    }
}

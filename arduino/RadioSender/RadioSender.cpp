#include "RadioSender.h"

#include <RF24Network.h>
#include <RF24.h>
#include <RF24Mesh.h>
#include <SPI.h>

static const uint32_t NETWORK_TIMEOUT = 10000;

/***** Configure the chosen CE,CS pins *****/
RF24 radio(9, 10);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

struct payload_t {
    uint8_t value;
};

void setup() {
    Serial.begin(9600);
    Serial.println("start setup");
    mesh.setNodeID(1);
    mesh.begin(MESH_DEFAULT_CHANNEL, RF24_1MBPS, NETWORK_TIMEOUT);
    Serial.println("done setup");
}

uint8_t value = 0;

void loop() {
    if (!mesh.checkConnection()) {
        Serial.println("reconnecting...");
        mesh.renewAddress(NETWORK_TIMEOUT);
    } else {
        payload_t payload = { value++ };
        RF24NetworkHeader header(0, OCT);
        Serial.println(value);
        network.write(header, &payload, sizeof(payload));
    }
    delay(1000);
}

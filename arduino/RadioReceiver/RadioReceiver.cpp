#include "RadioReceiver.h"

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
    mesh.setNodeID(0);
    mesh.begin(MESH_DEFAULT_CHANNEL, RF24_1MBPS, NETWORK_TIMEOUT);
    Serial.println("done setup");
}

void loop() {

    Serial.println("before update");
    // Call mesh.update to keep the network updated
    mesh.update();
    // In addition, keep the 'DHCP service' running on the master node so addresses will
    // be assigned to the sensor nodes
    Serial.println("before dhcp");
    mesh.DHCP();

    Serial.println("start reading");
    // Check for incoming data from the sensors
    if (network.available()) {
        Serial.println("got something");
        RF24NetworkHeader header;
        network.peek(header);
        uint8_t dat = 0;
        Serial.println("H: " + header.type);
        switch (header.type) {
        case 'M':
            Serial.println(header.type + ": ");
            network.read(header, &dat, sizeof(dat));
            Serial.println(dat);
            break;
        default:
            network.read(header, 0, 0);
            Serial.println(header.type + ": ");
            break;
        }
    } else {
        Serial.println("got something");
    }
}

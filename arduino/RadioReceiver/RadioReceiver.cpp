#include "RadioReceiver.h"

#include <RF24.h>
#include <aJSON.h>


// RF
static const uint64_t RF_PIPE_BASE = 0xE8E8F0F0A2LL;
static const uint8_t RF_PACKET_LENGTH = 32;

static const uint8_t JSON_MAX_READ_SIZE = 255;

RF24 radio(5, 6);

void setup() {
    Serial.begin(9600);

    radio.begin();
    radio.enableDynamicPayloads();
    for (int i = 1; i <= 16; i++) {
        radio.openReadingPipe(i, RF_PIPE_BASE + i);
    }
    radio.startListening();
    radio.powerUp();
}

void loop() {
    Serial.print(".");
    if (radio.available()) {
        char* msg = rfRead();
        aJsonObject *root;
        root = aJson.parse(msg);
        if (root != NULL) {
            Serial.println(msg);
        } else {
            Serial.println("invalid");
        }
        free(root);
        free(msg);
    }
    delay(1000);
}

char* rfRead() {
    char* msg = (char*) malloc(JSON_MAX_READ_SIZE);
    char* packet = (char*) malloc(RF_PACKET_LENGTH + 1);
    uint8_t len = 0;
    bool eoi = false;
    while (!eoi && len < JSON_MAX_READ_SIZE) {
        uint8_t l = radio.getDynamicPayloadSize();
        radio.read(packet, l);
        packet[l] = '\0';
        for (uint8_t i = 0; i < l && !eoi; i++) {
            msg[len++] = packet[i];
            eoi = (packet[i] == '\n');
        }
        packet[0] = '\0';
    }
    free(packet);
    msg[len] = '\0';
    return msg;
}

#include "RadioReceiver.h"

#include <RF24.h>

const uint64_t pipe = 0xE8E8F0F0E1LL;
const uint8_t len = 1;
uint8_t buff[len];
uint8_t state1 = 1;
uint8_t state2 = 0;
uint8_t state3 = 1;
uint8_t state4 = 0;

RF24 radio(9, 10);

void setup() {
    Serial.begin(9600);

    radio.begin();
    radio.openReadingPipe(1, pipe);

//    radio.setAutoAck(true);
    radio.startListening();
    radio.powerUp();

    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(7, OUTPUT);
}

void loop() {
    if (radio.available()) {
        bool eoi = false;
        while (!eoi) {
            eoi = radio.read(buff, sizeof(buff));
        }
        Serial.println(buff[0]);
        state1 = state1 ^ 1;
        state2 = state2 ^ 1;
        state3 = state3 ^ 1;
        state4 = state4 ^ 1;
        digitalWrite(4, state1);
        digitalWrite(5, state2);
        digitalWrite(6, state3);
        digitalWrite(7, state4);
    }
}

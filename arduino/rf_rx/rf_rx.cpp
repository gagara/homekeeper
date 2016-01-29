#include "rf_rx.h"

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

    pinMode(5, OUTPUT);
}

void loop() {
    digitalWrite(5, HIGH);
    delay(100);
    digitalWrite(5, LOW);
    if (radio.available()) {

        digitalWrite(5, HIGH);
        delay(100);
        digitalWrite(5, LOW);
        delay(100);
        digitalWrite(5, HIGH);
        delay(100);
        digitalWrite(5, LOW);
        delay(100);
        digitalWrite(5, HIGH);
        delay(100);
        digitalWrite(5, LOW);

        bool eoi = false;
        while (!eoi) {
            eoi = radio.read(buff, sizeof(buff));
        }
    }
    delay(1000);
}

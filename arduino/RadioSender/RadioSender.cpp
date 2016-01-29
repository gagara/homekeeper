#include "RadioSender.h"

#include <RF24.h>

const uint64_t pipe = 0xE8E8F0F0E1LL;

const uint8_t len = 1;
uint8_t buff[len];

RF24 radio(9, 10);

void setup() {
    Serial.begin(9600);
    buff[0] = HIGH;

    radio.begin();
    radio.openWritingPipe(pipe);
//    radio.powerUp();
}

void loop() {
    radio.write(buff, 1);
    Serial.print(".");
    delay(1000);
}

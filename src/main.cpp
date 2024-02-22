#include <Arduino.h>
#include <Packet.h>

static char* toHex(uint8_t value);

void setup() {

}

void loop() {
    delay(1);
}

static const char* hex = "0123456789ABCDEF";

static char* toHex(uint8_t number){
  return new char[2]{hex[number/16], hex[number % 16]};
}


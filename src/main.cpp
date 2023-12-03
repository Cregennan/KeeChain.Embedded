#include <Arduino.h>
#include <Packet.hpp>

char* toHex(uint8_t value);

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
  auto length = 6;
  uint8_t content[] = {0xBA, 0xBE, 0xBA, 0xBE, 0xBA, 0xBE};

  auto writer = [](uint8_t value){
    Serial.write(toHex(value));
    Serial.write(' ');
  };

  write_data(std::unique_ptr<uint8_t>(content), length, writer);
}

static const char* hex = "0123456789ABCDEF";

char* toHex(uint8_t number){
  return new char[2]{hex[number/16], hex[number % 16]};
}

void loop() {
    delay(1);
}
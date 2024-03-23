#include <Warlin.h>
#include <Arduino.h>

Warlin_ Warlin;

void discoverHandler(std::deque<std::string> & params);

void setup() {
    Serial.begin(DEFAULT_BAUDRATE);
    while(!Serial)
    {

    }

    Warlin.bind(PROTOCOL_REQUEST_TYPE::DISCOVER, discoverHandler);
}

void loop() {
    if (Warlin.available())
    {
        Warlin.process();
    }

    delay(100);
}

void discoverHandler(std::deque<std::string> & params)
{
   Warlin.writeString("HELLO");
}



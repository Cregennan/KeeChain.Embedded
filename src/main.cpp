#include <Arduino.h>
#include "Salavat.h"
#include "Warlin.h"

Warlin_ Warlin;
Salavat_ Salavat;

void discoverHandler(std::deque<std::string> & params);
void syncHandler(std::deque<std::string> & params);

void setup() {
    Serial.begin(DEFAULT_BAUDRATE);
    while(!Serial)
    {

    }

    Warlin.bind(PROTOCOL_REQUEST_TYPE::DISCOVER, discoverHandler);
    Warlin.bind(PROTOCOL_REQUEST_TYPE::SYNC, syncHandler);
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
   Warlin.writeLine(NameOf(PROTOCOL_RESPONSE_TYPE::ACK));
}

void syncHandler(std::deque<std::string> & params)
{
    if (params.empty())
    {
        SendErrorMessage("Malformed request: 0 tokens provided, 1 required");
        return;
    }
    auto result = Salavat.Initialize(millis(), std::stoi(params[0]));
    Warlin.writeLine({"SYNCR"});
}



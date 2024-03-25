#include <Warlin.h>
#include <Arduino.h>

Warlin_ Warlin;
time_t device_time;
time_t sync_millis;

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
    device_time = std::stoi(params[0]);
    sync_millis = millis();
    Warlin.writeLine({"SYNCR"});
}



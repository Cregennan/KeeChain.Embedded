#include <Arduino.h>
#include "Salavat.h"
#include "Warlin.h"

Warlin_ Warlin;
Salavat_ Salavat;

void discoverHandler(std::deque<std::string> & params);
void syncHandler(std::deque<std::string> & params);
void serviceEEPROMHandler(std::deque<std::string> & params);
void unlockHandler(std::deque<std::string> & params);
void getStoredNamesHandler(std::deque<std::string> & params);
void storeSecretHandler(std::deque<std::string> & params);

void setup() {
    Serial.begin(DEFAULT_BAUDRATE);
    while(!Serial)
    {

    }

    Warlin.bind(PROTOCOL_REQUEST_TYPE::DISCOVER, discoverHandler);
    Warlin.bind(PROTOCOL_REQUEST_TYPE::SYNC, syncHandler);
    Warlin.bind(PROTOCOL_REQUEST_TYPE::SERVICE_TRY_READ_EEPROM, serviceEEPROMHandler);
    Warlin.bind(PROTOCOL_REQUEST_TYPE::UNLOCK, unlockHandler);
    Warlin.bind(PROTOCOL_REQUEST_TYPE::GET_ENTRIES, getStoredNamesHandler);
    Warlin.bind(PROTOCOL_REQUEST_TYPE::STORE_ENTRY, storeSecretHandler);
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

    if (result == VAULT_INIT_RESULT::MALFORMED){
        Warlin.writeLine({ NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), "INIT_MALFORMED" });
        return;
    }

    Warlin.writeLine({ NameOf(PROTOCOL_RESPONSE_TYPE::SYNCR), std::to_string(Salavat.secretsCount()) });
}

void unlockHandler(std::deque<std::string> & params){
    const auto& reflector = EnumReflector::For<VAULT_UNLOCK_RESULT>();

    if (params.size() != 1){
        Warlin.writeLine({NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), "NOT_ENOUGH_PARAMS"});
        return;
    }
    auto password = params[0];
    auto unlockResult = Salavat.unlock(password);
    if (unlockResult != VAULT_UNLOCK_RESULT::SUCCESS){
        auto unlockResultValue = static_cast<uint8_t>(unlockResult);
        Warlin.writeLine({NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), reflector[unlockResultValue].Name()});
        return;
    }

    Warlin.writeLine(NameOf(PROTOCOL_RESPONSE_TYPE::ACK));
}

void serviceEEPROMHandler(std::deque<std::string> & params){
    static const char* hex = "0123456789ABCDEF";
    auto t = Salavat._service_read_eeprom_header();
    SendDebugMessage("EEPROM READ COMPLETED, COUNT: ", std::to_string(t.size()).c_str());
    std::string result;

    for (const auto &item: t){
        result.append(std::to_string(item));
        result.append(" ");
    }
    SendDebugMessage(result.c_str());

    Warlin.writeLine({NameOf(PROTOCOL_RESPONSE_TYPE::SERVICE)});
}

void getStoredNamesHandler(std::deque<std::string> & params){
    auto names = Salavat.getEntryNames();
    Warlin.writeLine(PROTOCOL_RESPONSE_TYPE::ENTRIES, names);
}

void storeSecretHandler(std::deque<std::string> & params){
    auto& reflector = EnumReflector::For<VAULT_ADD_ENTRY_RESULT>();

    if (params.size() < 3){
        Warlin.writeLine({ NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), "NOT_ENOUGH_PARAMS" });
        return;
    }
    auto digits = std::stoi(params[2]);
    auto result = Salavat.addEntry(params[0], params[1], digits);
    if (result != VAULT_ADD_ENTRY_RESULT::SUCCESS){
        auto& name = reflector[static_cast<uint8_t>(result)].Name();
        Warlin.writeLine({NameOf(PROTOCOL_RESPONSE_TYPE::ERROR), name});
        return;
    }
    Warlin.writeLine(PROTOCOL_RESPONSE_TYPE::ACK);
}

//WARLIN<PART>DISCOVER
//WARLIN<PART>SYNC<PART>1714312264
